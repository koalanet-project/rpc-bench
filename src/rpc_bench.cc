#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <memory>
#include <thread>

#include "app.h"
#include "command_opts.h"
#include "cpu_stat.h"
#include "logging.h"
#include "prism/utils.h"

using rpc_bench::App;
using rpc_bench::CommandOpts;

void ShowUsage(const char* app) {
  // clang-format off
  using rpc_bench::kDefaultPort;
  using rpc_bench::kDefaultDataSize;
  using rpc_bench::kDefaultConcurrency;
  using rpc_bench::kDefaultThread;
  using rpc_bench::kDefaultPersistent;
  using rpc_bench::kDefaultTimeSec;
  using rpc_bench::kDefaultWarmupTimeSec;
  fprintf(stdout, "Usage:\n");
  fprintf(stdout, "  %s           start an RPC benchmark server\n", app);
  fprintf(stdout, "  %s <host>    connect to server at <host>\n", app);
  fprintf(stdout, "\nOptions:\n");
  fprintf(stdout, "  -p, --port=<int>    the port to listen on or connect to, (default %d)\n", kDefaultPort);
  fprintf(stdout, "  -a, --app=<str>     benchmark app, a string in ['bandwidth', 'latency', 'throughput', 'hotel_reservation']\n");
  fprintf(stdout, "  -r, --rpc=<str>     rpc library, a string in ['grpc', 'socket', 'thrift', 'brpc']\n");
  fprintf(stdout, "  -d, --data=<size>   additional data size per request, (default %ld)\n", kDefaultDataSize);
  fprintf(stdout, "  -C, --concurrency=<int>   number of concurrent RPCs, (default %d)\n", kDefaultConcurrency);
  fprintf(stdout, "  -T, --thread=<int>   number of threads, (default %d)\n", kDefaultThread);
  fprintf(stdout, "  --monitor-time=<int>  # time in seconds to monitor cpu utilization\n");
  fprintf(stdout, "  --warmup=<int>      # time in seconds to wait before start monitoring cpu, (default %d secs if monitor-time is set)\n", kDefaultWarmupTimeSec);
  fprintf(stdout, "\nServer specific:\n");
  fprintf(stdout, "  --persistent        persistent server, (default %s)\n", kDefaultPersistent ? "true" : "false");
  fprintf(stdout, "\nClient specific:\n");
  fprintf(stdout, "  -P, --proto=<file>  protobuf format file\n");
  fprintf(stdout, "  -t, --time=<int>    # time in seconds to transmit for, (default %d secs)\n", kDefaultTimeSec);
  // clang-format on
}

int ParseArgument(int argc, char* argv[], CommandOpts* opts) {
  enum Tag {
    kPersistentTag = 1000,
    kWarmupTag,
    kMonitorTimeTag,
  };
  int err = 0;
  static struct option long_options[] = {// clang-format off
      {"help", no_argument, 0, 'h'},
      {"port", required_argument, 0, 'p'},
      {"app", required_argument, 0, 'a'},
      {"rpc", required_argument, 0, 'r'},
      {"proto", required_argument, 0, 'P'},
      {"data", required_argument, 0, 'd'},
      {"concurrency", required_argument, 0, 'C'},
      {"thread", required_argument, 0, 'T'},
      {"persistent", no_argument, 0, kPersistentTag},
      {"time", required_argument, 0, 't'},
      {"monitor-time", required_argument, 0, kMonitorTimeTag},
      {"warmup", required_argument, 0, kWarmupTag},
      {0, 0, 0, 0}
  };  // clang-format on
  while (1) {
    int option_index = 0, c;
    c = getopt_long(argc, argv, "hp:a:r:P:d:C:T:t:", long_options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h': {
        ShowUsage(argv[0]);
        exit(0);
      } break;
      case 'p': {
        opts->port = std::stoi(optarg);
      } break;
      case 'a': {
        opts->app = optarg;
      } break;
      case 'r': {
        opts->rpc = optarg;
      } break;
      case 'P': {
        opts->proto = optarg;
      } break;
      case 'd': {
        opts->data_size = std::stoi(optarg);
      } break;
      case 'C': {
        opts->concurrency = std::stoi(optarg);
      } break;
      case 'T': {
        opts->thread = std::stoi(optarg);
      } break;
      case kPersistentTag: {
        opts->persistent = true;
      } break;
      case kMonitorTimeTag: {
        opts->monitor_time_sec = std::stof(optarg);
      } break;
      case kWarmupTag: {
        opts->monitor_warmup_sec = std::stof(optarg);
      } break;
      case 't': {
        opts->time_duration_sec = std::stoi(optarg);
      } break;
      case '?':
      default:
        err = 1;
        goto out1;
    }
  }
  if (optind < argc) {
    if (optind + 1 != argc) {
      err = 1;
      goto out1;
    }
    opts->is_server = false;
    opts->host = argv[optind];
  } else {
    opts->is_server = true;
  }
out1:
  return err;
}

void CpuMonitor(const CommandOpts& opts) {
  CpuStats cpu_stats;

  size_t warmup_ms = opts.monitor_warmup_sec * 1000;
  std::this_thread::sleep_for(std::chrono::milliseconds(warmup_ms));
  cpu_stats.Snapshot();

  auto monitor_ms = std::chrono::milliseconds((int64_t)opts.monitor_time_sec.value() * 1000);
  auto start = std::chrono::high_resolution_clock::now();
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    cpu_stats.Snapshot();
    printf("CPU utilization: %.2lf%%\n", cpu_stats.CpuUtil() * 100);
    RPC_LOG(DEBUG) << cpu_stats.DebugString();

    auto end = std::chrono::high_resolution_clock::now();
    if (end - start > monitor_ms) break;
  }
}

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);

  struct CommandOpts opts;
  if (ParseArgument(argc, argv, &opts)) {
    ShowUsage(argv[0]);
    return 1;
  }

  std::cout << "Parsed options: " << opts.DebugString() << std::endl;

  /// start cpu monitor
  if (opts.monitor_time_sec.has_value()) {
    std::thread(CpuMonitor, opts).detach();
  }
  /// create and run app
  auto app = std::unique_ptr<App>(App::Create(opts));  // copy opts
  return app->Run();
}
