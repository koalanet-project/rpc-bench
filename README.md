# RPC Benchmark

## Usage
```
build/rpc-bench --help
Usage:
  build/rpc-bench           start an RPC benchmark server
  build/rpc-bench <host>    connect to server at <host>

Options:
  -p, --port=<int>    the port to listen on or connect to, (default 18000)
  -a, --app=<str>     benchmark app, a string in ['bandwidth', 'latency', 'throughput']
  -r, --rpc=<str>     rpc library, a string in ['grpc', 'thrift', 'brpc']
  -P, --proto=<file>  protobuf format file
  -d, --data=<size>   additional data size per request, (default 0)
```

## Build
Simply
```
make -j 4
```
It will downloads the dependencies and build them automatically.
