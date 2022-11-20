#![feature(type_alias_impl_trait)]
use std::net::SocketAddr;
use std::path::PathBuf;
use std::sync::atomic::{AtomicUsize, Ordering};

use structopt::StructOpt;

use volo_grpc::{Request, Response};

pub mod gen {
    volo::include_service!("proto_gen.rs");
}
use gen::proto_gen::rpc_hello::{Greeter, GreeterServer};
use gen::proto_gen::rpc_hello::{HelloReply, HelloRequest};

#[derive(StructOpt, Debug, Clone)]
#[structopt(about = "Tonic RPC hello client")]
pub struct Args {
    /// The port number to use.
    #[structopt(short, long, default_value = "5000")]
    pub port: u16,

    #[structopt(short = "l", long, default_value = "error")]
    pub log_level: String,

    #[structopt(long)]
    pub log_dir: Option<PathBuf>,

    #[structopt(long, default_value = "8")]
    pub reply_size: usize,

    #[structopt(long, default_value = "128")]
    pub provision_count: usize,

    /// Number of server threads.
    #[structopt(long, default_value = "1")]
    pub num_server_threads: usize,
}

struct MyGreeter {
    replies: Vec<Response<HelloReply>>,
    count: AtomicUsize,
    args: Args,
}

#[volo::async_trait]
impl Greeter for MyGreeter {
    async fn say_hello(
        &self,
        _request: Request<HelloRequest>,
    ) -> Result<Response<HelloReply>, volo_grpc::Status> {
        // eprintln!("reply: {:?}", request);

        let my_count = self.count.fetch_add(1, Ordering::AcqRel);
        Ok(Response::new(
            self.replies[my_count % self.args.provision_count]
                .get_ref()
                .clone(),
        ))
    }
}

fn run_server(tid: usize, args: Args) -> Result<(), volo_grpc::Status> {
    let rt = tokio::runtime::Builder::new_current_thread()
        .enable_io()
        .build()?;

    rt.block_on(async {
        let mut replies = Vec::new();
        for _ in 0..args.provision_count {
            let mut buf = Vec::with_capacity(args.reply_size);
            buf.resize(args.reply_size, 43);
            let msg = Response::new(HelloReply { message: buf });
            replies.push(msg);
        }

        let port = args.port + tid as u16;
        let addr =
            volo::net::Address::from(format!("0.0.0.0:{}", port).parse::<SocketAddr>().unwrap());

        GreeterServer::new(MyGreeter {
            replies,
            count: AtomicUsize::new(0),
            args,
        })
        .run(addr)
        .await
        .unwrap();

        Ok(())
    })
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    std::thread::scope(|s| {
        let mut handles = Vec::new();
        let args = Args::from_args();
        eprintln!("args: {:?}", args);

        for tid in 1..args.num_server_threads {
            let args = args.clone();
            handles.push(s.spawn(move || run_server(tid, args)));
        }

        run_server(0, args).unwrap();
        Ok(())
    })
}
