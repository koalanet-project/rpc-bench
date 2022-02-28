#ifndef RPC_BENCH_TPUT_APP_H_
#define RPC_BENCH_TPUT_APP_H_

#include "app.h"

namespace rpc_bench {

class TputServerApp : public App {
 public:
  TputServerApp(CommandOpts opts) : App(opts) {}

  virtual void Init() = 0;

  virtual int Run() = 0;
};

class TputClientApp : public App {
 public:
  TputClientApp(CommandOpts opts) : App(opts) {}

  virtual void Init() = 0;

  virtual int Run() = 0;
};

}  // namespace rpc_bench

#endif