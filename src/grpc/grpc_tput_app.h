#ifndef RPC_BENCH_GRPC_TPUT_APP_H_
#define RPC_BENCH_GRPC_TPUT_APP_H_
#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>

#include "command_opts.h"
#include "grpc/grpc_lat_app.h"
#include "protos/lat_tput_app.grpc.pb.h"
#include "protos/lat_tput_app.pb.h"
#include "tput_app.h"

namespace rpc_bench {
namespace grpc {

using ::grpc::ClientAsyncResponseReader;
using ::grpc::ClientContext;
using ::grpc::CompletionQueue;
using ::grpc::Server;
using ::grpc::ServerAsyncResponseWriter;
using ::grpc::ServerBuilder;
using ::grpc::ServerCompletionQueue;
using ::grpc::ServerContext;
using ::grpc::Status;

using ::rpc_bench::lat_tput_app::Ack;
using ::rpc_bench::lat_tput_app::Data;
using ::rpc_bench::lat_tput_app::LatTputService;

class GrpcTputClientApp final : public TputClientApp {
 public:
  GrpcTputClientApp(CommandOpts opts) : TputClientApp(opts) {}

  virtual void Init() override;

  virtual int Run() override;

 private:
  struct AsyncClientCall {
    Ack ack;
    ClientContext context;
    Status status;
    std::unique_ptr<ClientAsyncResponseReader<Ack>> resp_reader;

    AsyncClientCall(const std::unique_ptr<LatTputService::Stub>& stub_, CompletionQueue* cq_,
                    Data& data) {
      this->resp_reader = stub_->PrepareAsyncSendData(&this->context, data, cq_);
      this->resp_reader->StartCall();
      this->resp_reader->Finish(&this->ack, &this->status, this);
    }
  };

  std::unique_ptr<LatTputService::Stub> stub_;
  CompletionQueue cq_;
};

typedef GrpcLatServerApp GrpcTputServerApp;

}  // namespace grpc
}  // namespace rpc_bench

#endif