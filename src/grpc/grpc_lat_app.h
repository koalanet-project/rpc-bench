#ifndef RPC_BENCH_GRPC_LAT_APP_H_
#define RPC_BENCH_GRPC_LAT_APP_H_
#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>

#include "command_opts.h"
#include "lat_app.h"
#include "protos/lat_tput_app.grpc.pb.h"
#include "protos/lat_tput_app.pb.h"

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
  LatTputService::AsyncService service_;
  std::unique_ptr<Server> server_;

  class AsyncServerCall {
    LatTputService::AsyncService* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    ServerAsyncResponseWriter<Ack> responder_;

    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;

    Data data_;
    Ack ack_;
    size_t data_size_;

   public:
    AsyncServerCall(LatTputService::AsyncService* service, ServerCompletionQueue* cq,
                    size_t data_size)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE), data_size_(data_size) {
      ack_.set_data(std::string(data_size_, 'b'));
      Proceed();
    }

    void Proceed();
  };
};

}  // namespace grpc
}  // namespace rpc_bench

#endif