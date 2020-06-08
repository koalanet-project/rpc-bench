#include "app.h"
#include "bw_app.h"
#include "grpc/grpc_bw_app.h"
#include "command_opts.h"
#include "logging.h"

namespace rpc_bench {

App* App::Create(CommandOpts opts) {
  CHECK(opts.app.has_value());
  CHECK(opts.rpc.has_value());
  if (opts.app.value() == "bandwidth") {
    if (opts.rpc.value() == "grpc") {
      return opts.is_server.value() ? static_cast<App*>(new grpc::GrpcBwServerApp(opts)) : static_cast<App*>(new grpc::GrpcBwClientApp(opts));
    }
  } else {
    RPC_UNIMPLEMENTED
  }
}

}  // namespace rpc_bench
