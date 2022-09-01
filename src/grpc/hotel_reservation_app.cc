#include "grpc/hotel_reservation_app.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

#include <limits>
#include <thread>

#include "meter.h"
#include "command_opts.h"
#include "prism/utils.h"
#include "logging.h"

namespace rpc_bench {
namespace grpc {

// HotelReservationClientApp
void HotelReservationClientApp::Init() {
  std::string remote_uri = opts_.host.value() + ":" + std::to_string(opts_.port);
  RPC_LOG(DEBUG) << "gRPC client is connecting to remote_uri: " << remote_uri;
  stub_ = std::make_unique<Reservation::Stub>(
      ::grpc::CreateChannel(remote_uri, ::grpc::InsecureChannelCredentials()));
}

int HotelReservationClientApp::Run() {
  Init();

  Request request;
  request.set_customername("danyang");
  request.add_hotelid();
  request.set_hotelid(0, kHotelId);
  request.set_indate("2022-09-01");
  request.set_outdate("2022-09-02");
  request.set_roomnumber(128);

  int req_cnt = 0;
  for (int i = 0; i < opts_.concurrency; i++) {
    new AsyncClientCall(stub_, &cq_, request);
    req_cnt++;
  }

  void* tag;
  bool ok = false;
  long count = 0, overlimit = 0;
  size_t rx_size = 0;
  bool timeout = false;
  int time_period = 1000, time_cnt = 0;
  auto time_dura = std::chrono::microseconds(static_cast<long>(opts_.time_duration_sec * 1e6));
  auto start = std::chrono::high_resolution_clock::now();

  Meter meter = Meter(1000, "meter", 1);
  while (req_cnt > 0 && cq_.Next(&tag, &ok)) {
    AsyncClientCall* call = static_cast<AsyncClientCall*>(tag);
    if (call->status.ok()) {
      count++;
      rx_size += call->result.ByteSizeLong();
      meter.AddQps(opts_.data_size, 1);
    } else
      overlimit++;
    delete call;
    req_cnt--;

    time_cnt += 1;
    if (GPR_UNLIKELY(time_cnt >= time_period)) {
      time_cnt = 0;
      auto now = std::chrono::high_resolution_clock::now();
      timeout = (now - start > time_dura);
    }

    if (GPR_LIKELY(!timeout)) {
      new AsyncClientCall(stub_, &cq_, request);
      req_cnt++;
    }
  }

    return 0;
}

// grpc server handler
void HotelReservationServerApp::AsyncServerCall::Proceed() {
  switch (status_) {
    case CREATE:
      status_ = PROCESS;
      service_->RequestMakeReservation(&ctx_, &request_, &responder_, cq_, cq_, this);
      break;

    case PROCESS:
      new AsyncServerCall(service_, cq_);
      status_ = FINISH;
      responder_.Finish(result_, Status::OK, this);
      break;

    case FINISH:
      delete this;
      break;
  }
}

// HotelReservationServerApp
void HotelReservationServerApp::Init() {
  std::string bind_uri = "0.0.0.0:" + std::to_string(opts_.port);

  ::grpc::EnableDefaultHealthCheckService(true);
  ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;

  int hardware_concurreny = std::thread::hardware_concurrency();
  int new_max_threads = prism::GetEnvOrDefault<int>("RPC_BENCH_GRPC_MAX_THREAD", hardware_concurreny);
  RPC_LOG(DEBUG) << "gRPC thread pool is set to use " << new_max_threads << " threads";

  ResourceQuota quota;
  quota.SetMaxThreads(new_max_threads);
  builder.SetResourceQuota(quota);
  builder.SetMaxMessageSize(std::numeric_limits<int>::max());
  builder.AddListeningPort(bind_uri, ::grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);


  cq_ = builder.AddCompletionQueue();
  server_ = std::unique_ptr<Server>(builder.BuildAndStart());
  RPC_LOG(DEBUG) << "Async gRPC server is listening on uri: " << bind_uri;
}

// This can be run in multiple threads if needed.
void HotelReservationServerApp::HandleRpcs() {
  // Spawn a new AsyncServerCall instance to serve new clients.
  new AsyncServerCall(&service_, cq_.get());
  void* tag;  // uniquely identifies a request.
  bool ok;
  while (true) {
    // Block waiting to read the next event from the completion queue. The
    // event is uniquely identified by its tag, which in this case is the
    // memory address of a CallData instance.
    // The return value of Next should always be checked. This return value
    // tells us whether there is any kind of event or cq_ is shutting down.
    GPR_ASSERT(cq_->Next(&tag, &ok));
    static_cast<AsyncServerCall*>(tag)->Proceed();
  }
}

int HotelReservationServerApp::Run() {
  Init();
  HandleRpcs();
  return 0;
}

}  // namespace grpc
}  // namespace rpc_bench
