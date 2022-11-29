#ifndef RPC_BENCH_COMMAND_OPTS_H_
#define RPC_BENCH_COMMAND_OPTS_H_
#include <optional>
#include <sstream>
#include <string>

namespace rpc_bench {

// TODO(cjr): put these inside CommandsOpts
const int kDefaultPort = 18000;
const size_t kDefaultDataSize = 0;
const int kDefaultConcurrency = 32;
const int kDefaultThread = 1;
const bool kDefaultPersistent = false;
const int kDefaultTimeSec = 10;
const int kDefaultWarmupTimeSec = 1;
const bool kDefaultXdsServer = false;

struct CommandOpts {
  std::optional<bool> is_server;
  std::optional<std::string> host;
  int port;
  std::optional<std::string> app;
  std::optional<std::string> rpc;
  std::optional<std::string> proto;
  size_t data_size;
  int concurrency;
  int thread;
  bool persistent;
  double time_duration_sec;
  std::optional<double> monitor_time_sec;
  double monitor_warmup_sec;
  bool xds;

  CommandOpts() {
    port = kDefaultPort;
    data_size = kDefaultDataSize;
    concurrency = kDefaultConcurrency;
    persistent = kDefaultPersistent;
    time_duration_sec = kDefaultTimeSec;
    monitor_warmup_sec = kDefaultWarmupTimeSec;
    thread = kDefaultThread;
    xds = false;
  }

  // clang-format off
  std::string DebugString() {
    const char* NULLOPT = "(nullopt)";
    std::stringstream ss;
    ss << "{ is_server: " << is_server.value_or(NULLOPT) << ", host: " << host.value_or(NULLOPT)
       << ", port: " << port << ", app: " << app.value_or(NULLOPT)
       << ", rpc: " << rpc.value_or(NULLOPT) << ", proto: " << proto.value_or(NULLOPT)
       << ", data_size: " << data_size << ", concurrency: " << concurrency << ", thread: " << thread
       << ", persistent: " << persistent << ", time: " << time_duration_sec << ", monitor-time: "
       << (monitor_time_sec.has_value() ? std::to_string(monitor_time_sec.value()) : NULLOPT)
       << ", warmup: " << monitor_warmup_sec
       << ", xds: " << xds << " }";
    return ss.str();
  }
  // clang-format on
};

}  // namespace rpc_bench

#endif  // RPC_BENCH_COMMAND_OPTS_H_
