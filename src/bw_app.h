#ifndef RPC_BENCH_BW_APP_H_
#define RPC_BENCH_BW_APP_H_
#include "app.h"
#include "command_opts.h"
#include "meter.h"

namespace rpc_bench {

struct BwHeader {
  uint32_t field1;
  bool field2;
  std::string field3;
  size_t Size() const { return sizeof(field1) + sizeof(field2) + field3.size(); }
};

struct BwMessage {
  BwHeader header;
  std::string data;

  size_t Size() const { return header.Size() + data.size(); }
};

struct BwAck {
  BwHeader header;
  bool success;
};

class BwServerApp : public App {
 public:
  BwServerApp(CommandOpts opts) : App(opts) {}

  virtual int Run() = 0;

  virtual void Init() = 0;

 private:
};

class BwClientApp : public App {
 public:
  BwClientApp(CommandOpts opts) : App(opts), meter_{1000} {}

  virtual int Run() override;

  virtual void Init() = 0;

  virtual void IssueBwReq(const BwMessage& bw_msg, BwAck *bw_ack) = 0;

 private:
  Meter meter_;
};

}  // namespace rpc_bench

#endif  // RPC_BENCH_BW_APP_H_
