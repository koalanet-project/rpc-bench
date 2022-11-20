use std::path::PathBuf;
use std::sync::atomic::{AtomicUsize, Ordering};

use bytes::BytesMut;
use structopt::StructOpt;

use tonic::{Request, Response};

pub mod rpc_hello {
    tonic::include_proto!("rpc_hello");
}
use rpc_hello::greeter_server::{Greeter, GreeterServer};
use rpc_hello::{HelloReply, HelloRequest};

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

#[derive(Debug)]
struct MyGreeter {
    replies: Vec<Response<HelloReply>>,
    count: AtomicUsize,
    args: Args,
}

#[tonic::async_trait]
impl Greeter for MyGreeter {
    async fn say_hello(
        &self,
        _request: Request<HelloRequest>,
    ) -> Result<Response<HelloReply>, tonic::Status> {
        // eprintln!("reply: {:?}", request);

        let my_count = self.count.fetch_add(1, Ordering::AcqRel);
        Ok(Response::new(
            self.replies[my_count % self.args.provision_count]
                .get_ref()
                .clone(),
        ))
    }
}

fn run_server(tid: usize, args: Args) -> Result<(), tonic::Status> {
    let rt = tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()?;
    rt.block_on(async {
        let mut replies = Vec::new();
        for _ in 0..args.provision_count {
            let mut buf = BytesMut::with_capacity(args.reply_size);
            buf.resize(args.reply_size, 43);
            let msg = Response::new(HelloReply {
                message: buf.freeze(),
            });
            replies.push(msg);
        }

        let port = args.port + tid as u16;
        tonic::transport::Server::builder()
            .add_service(GreeterServer::new(MyGreeter {
                replies,
                count: AtomicUsize::new(0),
                args,
            }))
            .serve(format!("0.0.0.0:{}", port).parse().unwrap())
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
        let _guard = init_tokio_tracing(&args.log_level, &args.log_dir);

        for tid in 1..args.num_server_threads {
            let args = args.clone();
            handles.push(s.spawn(move || run_server(tid, args)));
        }

        run_server(0, args).unwrap();
        Ok(())
    })
}

fn init_tokio_tracing(
    level: &str,
    log_directory: &Option<PathBuf>,
) -> tracing_appender::non_blocking::WorkerGuard {
    let format = tracing_subscriber::fmt::format()
        .with_level(true)
        .with_target(true)
        .with_thread_ids(false)
        .with_thread_names(false)
        .compact();

    let env_filter = tracing_subscriber::filter::EnvFilter::builder()
        .parse(level)
        .expect("invalid tracing level");

    let (non_blocking, appender_guard) = if let Some(log_dir) = log_directory {
        let file_appender = tracing_appender::rolling::minutely(log_dir, "rpc-server.log");
        tracing_appender::non_blocking(file_appender)
    } else {
        tracing_appender::non_blocking(std::io::stdout())
    };

    tracing_subscriber::fmt::fmt()
        .event_format(format)
        .with_writer(non_blocking)
        .with_env_filter(env_filter)
        .init();

    tracing::info!("tokio_tracing initialized");

    appender_guard
}
