#include "app.h"

#include "brpc/brpc_bw_app.h"
#include "bw_app.h"
#include "command_opts.h"
#include "grpc/grpc_bw_app.h"
#include "grpc/grpc_lat_app.h"
#include "logging.h"
#include "socket/socket_bw_app.h"
#include "zeromq/zeromq_bw_app.h"

namespace rpc_bench {

App* App::Create(CommandOpts opts) {
  CHECK(opts.app.has_value());
  CHECK(opts.rpc.has_value());
  if (opts.app.value() == "bandwidth") {
    if (opts.rpc.value() == "grpc") {
      return opts.is_server.value() ? static_cast<App*>(new grpc::GrpcBwServerApp(opts))
                                    : static_cast<App*>(new grpc::GrpcBwClientApp(opts));
    } else if (opts.rpc.value() == "socket") {
      return opts.is_server.value() ? static_cast<App*>(new socket::SocketBwServerApp(opts))
                                    : static_cast<App*>(new socket::SocketBwClientApp(opts));
    } else if (opts.rpc.value() == "brpc") {
      return opts.is_server.value() ? static_cast<App*>(new brpc::BrpcBwServerApp(opts))
                                    : static_cast<App*>(new brpc::BrpcBwClientApp(opts));
    } else if (opts.rpc.value() == "zeromq") {
      return opts.is_server.value() ? static_cast<App*>(new zeromq::ZeromqBwServerApp(opts))
                                    : static_cast<App*>(new zeromq::ZeromqBwClientApp(opts));
    }
  } else if (opts.app.value() == "latency") {
    if (opts.rpc.value() == "grpc") {
      //   return opts.is_server.value() ? static_cast<App*>(new grpc::GrpcLatServerApp(opts))
      //                                 : static_cast<App*>(new grpc::GrpcLatClientApp(opts));
      return static_cast<App*>(new grpc::GrpcLatClientApp(opts));
    } else {
      RPC_UNIMPLEMENTED
    }
  } else if (opts.app.value() == "throughput") {
      RPC_UNIMPLEMENTED
    // if (opts.rpc.value() == "grpc") {
    //   return opts.is_server.value() ? static_cast<App*>(new grpc::GrpcTputServerApp(opts))
    //                                 : static_cast<App*>(new grpc::GrpcTputClientApp(opts));
    // } else {
    //   RPC_UNIMPLEMENTED
    // }
  }
}

}  // namespace rpc_bench
