#ifndef RPC_BENCH_BRPC_BW_APP_H_
#define RPC_BENCH_BRPC_BW_APP_H_
#include <brpc/channel.h>
#include <brpc/server.h>

#include "protos/bw_app.pb.h"
#include "protos/bw_app_brpc.pb.h"
#include "bw_app.h"
#include "command_opts.h"

namespace rpc_bench {
namespace brpc {

using ::rpc_bench::BwAck;
using ::rpc_bench::BwHeader;
using ::rpc_bench::BwMessage;
using ::rpc_bench::bw_app::PbBwAck;
using ::rpc_bench::bw_app::PbBwHeader;
using ::rpc_bench::bw_app::PbBwMessage;
using ::rpc_bench::bw_app::StopRequest;
using ::rpc_bench::bw_app::StopResponse;
using ::rpc_bench::bw_app::BwServiceBrpc_Stub;

class BrpcBwClientApp final : public BwClientApp {
 public:
  BrpcBwClientApp(CommandOpts opts) : BwClientApp(opts) {}

  virtual void Init() override;

  virtual void PushData(const BwMessage& bw_msg, BwAck* bw_ack) override;

 private:
  size_t log_id_;
  ::brpc::Channel channel_;
  std::unique_ptr<BwServiceBrpc_Stub> stub_;
};

using ::brpc::Server;
using ::rpc_bench::bw_app::BwServiceBrpc;
using ::google::protobuf::RpcController;
using ::google::protobuf::Closure;

// Logic and data behind the server's behavior.
class BwServiceImpl final : public BwServiceBrpc {
 public:
  BwServiceImpl() : meter_{1000} {}
  virtual ~BwServiceImpl() {}
  virtual void PushData(RpcController* cntl_base, const PbBwMessage* request, PbBwAck* reply, Closure* done) override;
  virtual void StopServer(RpcController* cntl_base, const StopRequest* req, StopResponse* res, Closure* done) override;
  Meter meter_;
};

class BrpcBwServerApp final : public BwServerApp {
 public:
  BrpcBwServerApp(CommandOpts opts) : BwServerApp(opts) {}

  virtual void Init() override;

  virtual int Run() override;

 private:
  BwServiceImpl service_;
  Server server_;
};

}  // namespace brpc
}  // namespace rpc_bench

#endif  // RPC_BENCH_BRPC_BW_APP_H_
