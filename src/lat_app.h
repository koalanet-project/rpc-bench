#ifndef RPC_BENCH_LAT_APP_H_
#define RPC_BENCH_LAT_APP_H_

#include <vector>

#include "app.h"
#include "logging.h"

namespace rpc_bench {

class LatServerApp : public App {
 public:
  LatServerApp(CommandOpts opts) : App(opts) {}

  virtual void Init() = 0;

  virtual int Run() = 0;
};

class LatClientApp : public App {
 public:
  LatClientApp(CommandOpts opts) : App(opts) {}

  virtual void Init() = 0;

  virtual int Run() = 0;

  void print();

  std::vector<double> latencies;
};

}  // namespace rpc_bench

#endif