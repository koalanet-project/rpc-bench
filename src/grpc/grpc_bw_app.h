#ifndef RPC_BENCH_GRPC_BW_APP_H_
#define RPC_BENCH_GRPC_BW_APP_H_
#include <grpcpp/grpcpp.h>

#include <memory>

#include "bw_app.h"
#include "command_opts.h"
#include "grpcpp/health_check_service_interface.h"
#include "grpcpp/security/server_credentials.h"
#include "protos/bw_app.grpc.pb.h"
#include "protos/bw_app.pb.h"

namespace rpc_bench {
namespace grpc {

using ::grpc::Channel;
using ::grpc::ClientContext;
using ::grpc::Status;
using ::rpc_bench::BwAck;
using ::rpc_bench::BwHeader;
using ::rpc_bench::BwMessage;
using ::rpc_bench::bw_app::BwService;
using ::rpc_bench::bw_app::PbBwAck;
using ::rpc_bench::bw_app::PbBwHeader;
using ::rpc_bench::bw_app::PbBwMessage;
using ::rpc_bench::bw_app::StopRequest;
using ::rpc_bench::bw_app::StopResponse;

class GrpcBwClientApp final : public BwClientApp {
 public:
  GrpcBwClientApp(CommandOpts opts) : BwClientApp(opts) {}

  virtual void Init() override;

  virtual void PushData(const BwMessage& bw_msg, BwAck* bw_ack) override;

 private:
  std::unique_ptr<BwService::Stub> stub_;
};

using ::grpc::Server;
using ::grpc::ServerBuilder;
using ::grpc::ServerContext;
using ::grpc::ResourceQuota;

// Logic and data behind the server's behavior.
class BwServiceImpl final : public BwService::Service {
 public:
  BwServiceImpl() : meter_{1000} {}
  Status PushData(ServerContext* context, const PbBwMessage* request, PbBwAck* reply) override;
  Status StopServer(ServerContext* context, const StopRequest* req, StopResponse* res) override;
  Meter meter_;
};

class GrpcBwServerApp final : public BwServerApp {
 public:
  GrpcBwServerApp(CommandOpts opts) : BwServerApp(opts) {}

  virtual void Init() override;

  virtual int Run() override;

 private:
  BwServiceImpl service_;
  std::unique_ptr<Server> server_;
};

}  // namespace grpc
}  // namespace rpc_bench

#endif  // RPC_BENCH_GRPC_BW_APP_H_
