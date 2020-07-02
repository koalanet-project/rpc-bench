#ifndef RPC_BENCH_ZEROMQ_BW_APP_H_
#define RPC_BENCH_ZEROMQ_BW_APP_H_

#include <memory>
#include <zmq.h>

#include "bw_app.h"
#include "command_opts.h"

namespace rpc_bench {
namespace zeromq {

using ::rpc_bench::BwAck;
using ::rpc_bench::BwHeader;
using ::rpc_bench::BwMessage;

class ZeromqBwClientApp final : public BwClientApp {
 public:
  ZeromqBwClientApp(CommandOpts opts) : BwClientApp(opts) {}

  virtual void Init() override;

  virtual void PushData(const BwMessage& bw_msg, BwAck* bw_ack) override;

  void PushDataZero(const BwMessage& bw_msg, BwAck* bw_ack);

 private:
  void* zmq_context_;
  void* zmq_requester_;
  char recv_buffer_[100];
};

class ZeromqBwServerApp final : public BwServerApp {
 public:
  ZeromqBwServerApp(CommandOpts opts) : BwServerApp(opts) {}

  virtual void Init() override;

  virtual int Run() override;
  int Run2();

 private:
  void* zmq_context_;
  void* zmq_responder_;
};

}  // namespace zeromq
}  // namespace rpc_bench

#endif  // RPC_BENCH_ZEROMQ_BW_APP_H_
