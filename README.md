# RPC Benchmark

## Usage
```
Usage:
  build/rpc-bench           start an RPC benchmark server
  build/rpc-bench <host>    connect to server at <host>

Options:
  -p, --port=<int>    the port to listen on or connect to, (default 18000)
  -a, --app=<str>     benchmark app, a string in ['bandwidth', 'latency', 'throughput']
  -r, --rpc=<str>     rpc library, a string in ['grpc', 'socket', 'thrift', 'brpc']
  -d, --data=<size>   additional data size per request, (default 0)
  --monitor-time=<int>  # time in seconds to monitor cpu utilization
  --warmup=<int>      # time in seconds to wait before start monitoring cpu, (default 1 secs if monitor-time is set)

Server specific:
  --persistent        persistent server, (default false)

Client specific:
  -P, --proto=<file>  protobuf format file
  -t, --time=<int>    # time in seconds to transmit for, (default 10 secs)
```

## Build
Simply
```
make -j 4 DEBUG=0
```
It will downloads the dependencies and build them automatically.
