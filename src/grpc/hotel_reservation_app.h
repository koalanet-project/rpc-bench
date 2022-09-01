#ifndef RPC_BENCH_HOTEL_RESERVATION_APP_H_
#define RPC_BENCH_HOTEL_RESERVATION_APP_H_
#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>

#include <memory>

#include "command_opts.h"
#include "app.h"
#include "grpcpp/health_check_service_interface.h"
#include "grpcpp/security/server_credentials.h"
#include "protos/reservation.grpc.pb.h"
#include "protos/reservation.pb.h"

namespace rpc_bench {
namespace grpc {

using ::grpc::Status;
using ::grpc::ClientAsyncResponseReader;
using ::grpc::ClientContext;
using ::grpc::CompletionQueue;

using ::reservation::Reservation;
using ::reservation::Request;
using ::reservation::Result;

class HotelReservationClientApp : public App {
 public:
  HotelReservationClientApp(CommandOpts opts) : App(opts) {}

  void Init();

  virtual int Run() override;

 private:
  struct AsyncClientCall {
    Result result;
    size_t size;
    ClientContext context;
    Status status;
    std::unique_ptr<ClientAsyncResponseReader<Result>> resp_reader;

    AsyncClientCall(const std::unique_ptr<Reservation::Stub>& stub, CompletionQueue* cq,
                    Request& data, size_t size) {
      this->resp_reader = stub->PrepareAsyncMakeReservation(&this->context, data, cq);
      this->resp_reader->StartCall();
      this->resp_reader->Finish(&this->result, &this->status, this);
      this->size = size;
    }
  };

  std::unique_ptr<Reservation::Stub> stub_;
  CompletionQueue cq_;
};

using ::grpc::Server;
using ::grpc::ServerBuilder;
using ::grpc::ServerContext;
using ::grpc::ResourceQuota;
using ::grpc::ServerAsyncResponseWriter;
using ::grpc::ServerCompletionQueue;

const std::string kHotelId = "42";

class HotelReservationServerApp final : public App {
 public:
  HotelReservationServerApp(CommandOpts opts) : App(opts) {}

  void Init();

  virtual int Run() override;

  void HandleRpcs();

 private:
  std::unique_ptr<ServerCompletionQueue> cq_;
  Reservation::AsyncService service_;
  std::unique_ptr<Server> server_;

  class AsyncServerCall {
    Reservation::AsyncService* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    ServerAsyncResponseWriter<Result> responder_;

    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;
  
    Request request_;
    Result result_;
  
   public:
    AsyncServerCall(Reservation::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
      result_.add_hotelid();
      result_.set_hotelid(0, kHotelId);
      Proceed();
    }

    void Proceed();
  };
};

}  // namespace grpc
}  // namespace rpc_bench

#endif  // RPC_BENCH_HOTEL_RESERVATION_APP_H_