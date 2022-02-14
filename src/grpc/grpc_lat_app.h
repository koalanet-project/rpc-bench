#ifndef RPC_BENCH_GRPC_LAT_APP_H_
#define RPC_BENCH_GRPC_LAT_APP_H_
#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>

#include "command_opts.h"
#include "lat_app.h"
#include "protos/lat_app.grpc.pb.h"
#include "protos/lat_app.pb.h"

namespace rpc_bench {
namespace grpc {

using ::grpc::ClientAsyncReaderWriter;
using ::grpc::ClientContext;
using ::grpc::Status;
using ::grpc::CompletionQueue;

using ::rpc_bench::lat_app::Ack;
using ::rpc_bench::lat_app::Data;
using ::rpc_bench::lat_app::LatService;

class GrpcLatClientApp final : public LatClientApp {
 public:
  GrpcLatClientApp(CommandOpts opts) : LatClientApp(opts) {}

  virtual void Init() override;

  virtual int Run() override;

 private:
  struct AsyncClientCall {
    Ack ack;
    ClientContext context;
    Status status;
    std::unique_ptr<ClientAsyncReaderWriter<Data, Ack>> stream;
    bool sendfinished;
    bool finished;
    bool writing;

    AsyncClientCall(const std::unique_ptr<LatService::Stub>& stub_, CompletionQueue* cq_) {
      this->stream = stub_->PrepareAsyncSendDataStreamFullDuplex(&this->context, cq_);
      this->stream->StartCall((void*)this);
      this->sendfinished = false;
      this->finished = false;
      this->writing = true;
    }
  };

  std::unique_ptr<LatService::Stub> stub_;
  CompletionQueue cq_;
};

}  // namespace grpc
}  // namespace rpc_bench

#endif
