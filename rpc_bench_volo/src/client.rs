#![feature(type_alias_impl_trait)]
use std::future::Future;
use std::net::ToSocketAddrs;
use std::path::PathBuf;
use std::time::Duration;

use futures::select;
use futures::stream::FuturesUnordered;
use futures::stream::StreamExt;
use hdrhistogram::Histogram;
use minstant::Instant;
use structopt::StructOpt;

use volo_grpc::Response;

pub mod gen {
    volo::include_service!("proto_gen.rs");
}
use gen::proto_gen::rpc_hello::{GreeterClient, GreeterClientBuilder};
use gen::proto_gen::rpc_hello::{HelloReply, HelloRequest};

#[derive(StructOpt, Debug)]
#[structopt(about = "Koala RPC hello client")]
pub struct Args {
    /// The address to connect, can be an IP address or domain name.
    #[structopt(short = "c", long = "connect", default_value = "192.168.211.194")]
    pub ip: String,

    /// The port number to use.
    #[structopt(short, long, default_value = "5000")]
    pub port: u16,

    /// Log level for tracing.
    #[structopt(short = "l", long, default_value = "error")]
    pub log_level: String,

    /// Log directory.
    #[structopt(long)]
    pub log_dir: Option<PathBuf>,

    #[structopt(long)]
    pub log_latency: bool,

    /// Request size.
    #[structopt(short, long, default_value = "1000000")]
    pub req_size: usize,

    /// The maximal number of concurrenty outstanding requests.
    #[structopt(long, default_value = "32")]
    pub concurrency: usize,

    /// Total number of iterations.
    #[structopt(short, long, default_value = "16384")]
    pub total_iters: usize,

    /// Number of warmup iterations.
    #[structopt(short, long, default_value = "1000")]
    pub warmup: usize,

    /// Run test for a customized period of seconds.
    #[structopt(short = "D", long)]
    pub duration: Option<f64>,

    /// Seconds between periodic throughput reports.
    #[structopt(short, long)]
    pub interval: Option<f64>,

    /// The number of messages to provision. Must be a positive number. The test will repeatedly
    /// sending the provisioned message.
    #[structopt(long, default_value = "1")]
    pub provision_count: usize,

    /// Number of client threads. Each client thread is mapped to one server threads.
    #[structopt(long, default_value = "1")]
    pub num_client_threads: usize,

    /// Number of server threads.
    #[structopt(long, default_value = "1")]
    pub num_server_threads: usize,
}

#[derive(Debug)]
struct RpcCall {
    start: Instant,
    req_size: usize,
    res: Result<Response<HelloReply>, volo_grpc::Status>,
}

fn make_call<'a>(
    scnt: usize,
    reqs: &'a [HelloRequest],
    client: &GreeterClient,
    args: &Args,
) -> impl Future<Output = RpcCall> + 'a {
    let start = Instant::now();
    // This clone is cheap because HelloRequest only contains a single bytes::Bytes.
    let req = reqs[scnt % args.provision_count].clone();
    let req_size = args.req_size;
    let mut client = client.clone();
    async move {
        let fut = client.say_hello(req);
        RpcCall {
            start,
            req_size,
            res: fut.await,
        }
    }
}

#[allow(unused)]
async fn run_bench(
    args: &Args,
    client: GreeterClient,
    mut reqs: Vec<HelloRequest>,
    tid: usize,
) -> Result<(Duration, usize, usize, Histogram<u64>), volo_grpc::Status> {
    let mut hist = hdrhistogram::Histogram::<u64>::new_with_max(60_000_000_000, 5).unwrap();

    let mut starts = Vec::with_capacity(args.concurrency);
    let now = Instant::now();
    starts.resize(starts.capacity(), now);

    let mut reply_futures = FuturesUnordered::new();

    let (total_iters, timeout) = if let Some(dura) = args.duration {
        (usize::MAX / 2, Duration::from_secs_f64(dura))
    } else {
        (args.total_iters, Duration::from_millis(u64::MAX))
    };

    // report the rps every several milliseconds
    let tput_interval = args.interval.map(Duration::from_secs_f64);

    // start sending
    let mut last_ts = Instant::now();
    let start = Instant::now();

    let mut warmup_end = Instant::now();
    let mut nbytes = 0;
    let mut last_nbytes = 0;

    let mut scnt = 0;
    let mut rcnt = 0;
    let mut last_rcnt = 0;

    while scnt < args.concurrency && scnt < total_iters + args.warmup {
        let fut = make_call(scnt, &reqs, &client, args);
        reply_futures.push(fut);
        scnt += 1;
    }

    loop {
        select! {
            resp = reply_futures.next() => {
                if rcnt >= total_iters + args.warmup || start.elapsed() > timeout {
                    break;
                }

                // let resp = resp.unwrap()?;
                let resp = resp.unwrap();
                let RpcCall { start, req_size, res } = resp;

                let dura = start.elapsed();
                if rcnt >= args.warmup {
                    let _ = hist.record(dura.as_nanos() as u64);
                }

                nbytes += req_size;
                rcnt += 1;
                if rcnt == args.warmup {
                    warmup_end = Instant::now();
                }

                if scnt < total_iters + args.warmup {
                    let fut = make_call(scnt, &reqs, &client, args);
                    reply_futures.push(fut);
                    scnt += 1;
                }

                let last_dura = last_ts.elapsed();
                if tput_interval.is_some() && last_dura > tput_interval.unwrap() {
                    let rps = (rcnt - last_rcnt) as f64 / last_dura.as_secs_f64();
                    let bw = 8e-9 * (nbytes - last_nbytes) as f64 / last_dura.as_secs_f64();
                    if rcnt>args.warmup{
                        if args.log_latency {
                            println!(
                                    "Thread {}, {} rps, {} Gb/s, p95: {:?}, p99: {:?}",
                                    tid,
                                    rps,
                                    bw,
                                    Duration::from_nanos(hist.value_at_percentile(95.0)),
                                    Duration::from_nanos(hist.value_at_percentile(99.0)),
                                );
                                hist.clear();
                        } else {
                            println!("Thread {}, {} rps, {} Gb/s", tid, rps, bw);
                        }
                    }
                    last_ts = Instant::now();
                    last_rcnt = rcnt;
                    last_nbytes = nbytes;
                }
            }
            complete => break,
            // default => {
            // // Having a default branch when no futures is ready causes tonic not to make any
            // // progress. This sounds like a bug of tonic.
            // }
        }
    }

    let dura = warmup_end.elapsed();
    Ok((dura, nbytes, rcnt, hist))
}

fn run_client_thread(tid: usize, args: &Args) -> Result<(), Box<dyn std::error::Error>> {
    let addr = (args.ip.as_str(), args.port + (tid % args.num_server_threads) as u16)
        .to_socket_addrs()
        .unwrap()
        .next()
        .unwrap();

    let rt = tokio::runtime::Builder::new_current_thread()
        .enable_io()
        .enable_time()
        .build()?;

    rt.block_on(async {
        let client = GreeterClientBuilder::new("hello")
            .address(addr)
            .build();
        eprintln!("connection setup for thread {tid}");

        // provision
        let mut reqs = Vec::new();
        for _ in 0..args.provision_count {
            let mut buf = Vec::with_capacity(args.req_size);
            buf.resize(args.req_size, 42);
            let req = HelloRequest { name: buf };
            reqs.push(req);
        }

        let (dura, total_bytes, rcnt, hist) = run_bench(&args, client, reqs, tid).await?;

        println!(
            "Thread {tid}, duration: {:?}, bandwidth: {:?} Gb/s, rate: {:.5} Mrps",
            dura,
            8e-9 * (total_bytes - args.warmup * args.req_size) as f64 / dura.as_secs_f64(),
            1e-6 * (rcnt - args.warmup) as f64 / dura.as_secs_f64(),
        );
        // print latencies
        println!(
            "Thread {tid}, duration: {:?}, avg: {:?}, min: {:?}, median: {:?}, p95: {:?}, p99: {:?}, max: {:?}",
            dura,
            Duration::from_nanos(hist.mean() as u64),
            Duration::from_nanos(hist.min()),
            Duration::from_nanos(hist.value_at_percentile(50.0)),
            Duration::from_nanos(hist.value_at_percentile(95.0)),
            Duration::from_nanos(hist.value_at_percentile(99.0)),
            Duration::from_nanos(hist.max()),
        );

        Result::<(), volo_grpc::Status>::Ok(())
    })?;

    Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = Args::from_args();
    eprintln!("args: {:?}", args);

    assert!(args.num_client_threads % args.num_server_threads == 0);

    std::thread::scope(|s| {
        let mut handles = Vec::new();
        for tid in 1..args.num_client_threads {
            let args = &args;
            handles.push(s.spawn(move || {
                run_client_thread(tid, args).unwrap();
            }));
        }
        run_client_thread(0, &args).unwrap();
    });

    Ok(())
}
