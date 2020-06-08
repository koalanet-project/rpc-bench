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

void PackPbBwMessage(const BwMessage& bw_msg, PbBwMessage* pb_bw_msg);

void PackPbBwHeader(const BwHeader bw_header, PbBwHeader* pb_bw_header);

void UnpackPbBwHeader(BwHeader* bw_header, const PbBwHeader& pb_bw_header);

class GrpcBwClientApp final : public BwClientApp {
 public:
  GrpcBwClientApp(CommandOpts opts) : BwClientApp(opts) {}

  virtual void Init() override;

  virtual void IssueBwReq(const BwMessage& bw_msg, BwAck* bw_ack) override;

 private:
  std::unique_ptr<BwService::Stub> stub_;
};

using ::grpc::Server;
using ::grpc::ServerBuilder;
using ::grpc::ServerContext;

// Logic and data behind the server's behavior.
class BwServiceImpl final : public BwService::Service {
 public:
  BwServiceImpl() : meter_{1000} {}
  Status Request(ServerContext* context, const PbBwMessage* request, PbBwAck* reply) override;
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
