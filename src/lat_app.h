#ifndef RPC_BENCH_LAT_APP_H_
#define RPC_BENCH_LAT_APP_H_

#include "app.h"

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
};

}  // namespace rpc_bench

#endif