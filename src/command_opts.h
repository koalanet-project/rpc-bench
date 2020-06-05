#ifndef RPC_BENCH_COMMAND_OPTS_H_
#define RPC_BENCH_COMMAND_OPTS_H_
#include <string>
#include <optional>
#include <sstream>

namespace rpc_bench {

const int kDefaultPort = 18000;
const size_t kDefaultDataSize = 0;

struct CommandOpts {
  std::optional<bool> is_server;
  std::optional<std::string> host;
  int port;
  std::optional<std::string> app;
  std::optional<std::string> rpc;
  std::optional<std::string> proto;
  size_t data_size;

  CommandOpts() {
    port = kDefaultPort;
    data_size = kDefaultDataSize;
  }

  std::string DebugString() {
    const char* NULLOPT = "(nullopt)";
    std::stringstream ss;
    ss << "{ is_server: " << is_server.value_or(NULLOPT)
       << ", host: " << host.value_or(NULLOPT)
       << ", port: " << port
       << ", app: " << app.value_or(NULLOPT)
       << ", rpc: " << rpc.value_or(NULLOPT)
       << ", proto: " << proto.value_or(NULLOPT)
       << ", data_size: " << data_size << " }";
    return ss.str();
  }
};

}  // namespace rpc_bench

#endif  // RPC_BENCH_COMMAND_OPTS_H_
