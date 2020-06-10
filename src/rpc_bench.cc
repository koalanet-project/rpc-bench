#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

#include "app.h"
#include "command_opts.h"

using rpc_bench::App;
using rpc_bench::CommandOpts;

void ShowUsage(const char* app) {
  // clang-format off
  using rpc_bench::kDefaultPort;
  using rpc_bench::kDefaultDataSize;
  using rpc_bench::kDefaultPersistent;
  using rpc_bench::kDefaultTimeSec;
  fprintf(stdout, "Usage:\n");
  fprintf(stdout, "  %s           start an RPC benchmark server\n", app);
  fprintf(stdout, "  %s <host>    connect to server at <host>\n", app);
  fprintf(stdout, "\nOptions:\n");
  fprintf(stdout, "  -p, --port=<int>    the port to listen on or connect to, (default %d)\n", kDefaultPort);
  fprintf(stdout, "  -a, --app=<str>     benchmark app, a string in ['bandwidth', 'latency', 'throughput']\n");
  fprintf(stdout, "  -r, --rpc=<str>     rpc library, a string in ['grpc', 'thrift', 'brpc']\n");
  fprintf(stdout, "  -d, --data=<size>   additional data size per request, (default %ld)\n", kDefaultDataSize);
  fprintf(stdout, "\nServer specific:\n");
  fprintf(stdout, "  --persistent        persistent server, (default %s)\n", kDefaultPersistent ? "true" : "false");
  fprintf(stdout, "\nClient specific:\n");
  fprintf(stdout, "  -P, --proto=<file>  protobuf format file\n");
  fprintf(stdout, "  -t, --time          # time in seconds to transmit for, (default %d secs)", kDefaultTimeSec);
  // clang-format on
}

int ParseArgument(int argc, char* argv[], CommandOpts* opts) {
  static const int kPersistentTag = 1000;
  int err = 0;
  static struct option long_options[] = { // clang-format off
      {"help", no_argument, 0, 'h'},
      {"port", required_argument, 0, 'p'},
      {"app", required_argument, 0, 'a'},
      {"rpc", required_argument, 0, 'r'},
      {"proto", required_argument, 0, 'P'},
      {"data", required_argument, 0, 'd'},
      {"persistent", no_argument, 0, kPersistentTag},
      {"time", required_argument, 0, 't'},
      {0, 0, 0, 0}
  };  // clang-format on
  while (1) {
    int option_index = 0, c;
    c = getopt_long(argc, argv, "hp:a:r:P:d:", long_options, &option_index);
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
      case kPersistentTag: {
        opts->persistent = true;
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

int main(int argc, char* argv[]) {
  struct CommandOpts opts;
  if (ParseArgument(argc, argv, &opts)) {
    ShowUsage(argv[0]);
    return 1;
  }
  std::cout << "Parsed options: " << opts.DebugString() << std::endl;
  auto app = std::unique_ptr<App>(App::Create(opts));  // copy opts
  return app->Run();
}
