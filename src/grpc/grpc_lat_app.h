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
using ::grpc::CompletionQueue;
using ::grpc::Server;
using ::grpc::ServerAsyncReaderWriter;
using ::grpc::ServerBuilder;
using ::grpc::ServerCompletionQueue;
using ::grpc::ServerContext;
using ::grpc::Status;

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

class GrpcLatServerApp final : public LatServerApp {
 public:
  GrpcLatServerApp(CommandOpts opts) : LatServerApp(opts) {}

  ~GrpcLatServerApp() {
    server_->Shutdown();
    cq_->Shutdown();
  }

  virtual void Init() override;

  virtual int Run() override;

 private:
  std::unique_ptr<ServerCompletionQueue> cq_;
  LatService::AsyncService service_;
  std::unique_ptr<Server> server_;

  class AsyncServerCall {
    LatService::AsyncService* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    ServerAsyncReaderWriter<Ack, Data> stream_;

    enum CallStatus { CREATE, PROCESS, READ_COMPLETE, WRITE_COMPLETE, FINISH };
    CallStatus status_;

    Data data_;
    Ack ack_;
    size_t data_size_;

   public:
    AsyncServerCall(LatService::AsyncService* service, ServerCompletionQueue* cq, size_t data_size)
        : service_(service), cq_(cq), stream_(&ctx_), status_(CREATE), data_size_(data_size) {
      ack_.set_data(std::string(data_size_, 'b'));
      Proceed(true);
    }

    void Proceed(bool ok);
  };
};

}  // namespace grpc
}  // namespace rpc_bench

#endif
