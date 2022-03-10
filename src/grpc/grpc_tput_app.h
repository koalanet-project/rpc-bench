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

using ::grpc::ClientAsyncReaderWriter;
using ::grpc::ClientContext;
using ::grpc::CompletionQueue;
using ::grpc::Server;
using ::grpc::ServerAsyncReaderWriter;
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
    enum CallStatus { CREATE, WRITE, READ, WRITEDONE, FINISH, CLOSED };

    Ack ack;
    ClientContext context;
    Status status;
    std::unique_ptr<ClientAsyncReaderWriter<Data, Ack>> stream;
    long count;
    CallStatus call_status;

    AsyncClientCall(const std::unique_ptr<LatTputService::Stub>& stub_, CompletionQueue* cq_) {
      this->stream = stub_->PrepareAsyncSendDataStreamFullDuplex(&this->context, cq_);
      this->stream->StartCall((void*)this);
      this->count = 0;
      this->call_status = CREATE;
    }
  };

  int call_per_req_;
  std::unique_ptr<LatTputService::Stub> stub_;
  CompletionQueue cq_;
};

typedef GrpcLatServerApp GrpcTputServerApp;

}  // namespace grpc
}  // namespace rpc_bench

#endif
