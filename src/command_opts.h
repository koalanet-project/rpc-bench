#ifndef RPC_BENCH_COMMAND_OPTS_H_
#define RPC_BENCH_COMMAND_OPTS_H_
#include <string>
#include <optional>
#include <sstream>

namespace rpc_bench {

// TODO(cjr): put these inside CommandsOpts
const int kDefaultPort = 18000;
const size_t kDefaultDataSize = 0;
const int kDefaultConcurrency = 1;
const bool kDefaultPersistent = false;
const int kDefaultTimeSec = 10;
const int kDefaultWarmupTimeSec = 1;

struct CommandOpts {
  std::optional<bool> is_server;
  std::optional<std::string> host;
  int port;
  std::optional<std::string> app;
  std::optional<std::string> rpc;
  std::optional<std::string> proto;
  size_t data_size;
  bool persistent;
  double time_duration_sec;
  std::optional<double> monitor_time_sec;
  double monitor_warmup_sec;

  CommandOpts() {
    port = kDefaultPort;
    data_size = kDefaultDataSize;
    persistent = kDefaultPersistent;
    time_duration_sec = kDefaultTimeSec;
    monitor_warmup_sec = kDefaultWarmupTimeSec;
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
       << ", data_size: " << data_size
       << ", persistent: " << persistent
       << ", time: " << time_duration_sec
       << ", monitor-time: " << (monitor_time_sec.has_value() ? std::to_string(monitor_time_sec.value()) : NULLOPT)
       << ", warmup: " << monitor_warmup_sec
       << " }";
    return ss.str();
  }
};

}  // namespace rpc_bench

#endif  // RPC_BENCH_COMMAND_OPTS_H_
