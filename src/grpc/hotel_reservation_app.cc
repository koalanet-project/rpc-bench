#include "grpc/hotel_reservation_app.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

#include <grpcpp/xds_server_builder.h>

#include <limits>
#include <thread>

#include "meter.h"
#include "command_opts.h"
#include "prism/utils.h"
#include "logging.h"

namespace rpc_bench {
namespace grpc {

static unsigned int g_seed;

// Used to seed the generator.
inline void fast_srand(int seed) {
    g_seed = seed;
}

// Compute a pseudorandom integer.
// Output value in range [0, 32767]
inline int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16)&0x7FFF;
}

// HotelReservationClientApp
void HotelReservationClientApp::Init() {
  std::string remote_uri = opts_.host.value() + ":" + std::to_string(opts_.port);
  RPC_LOG(DEBUG) << "gRPC client is connecting to remote_uri: " << remote_uri;
  stub_ = std::make_unique<Reservation::Stub>(
      ::grpc::CreateChannel(remote_uri, ::grpc::InsecureChannelCredentials()));
}

auto GenerateWorkload() -> std::tuple<Request&, size_t> {
  static bool init = false;
  static Request request[2];
  static size_t req_size[2];
  if (!init) {
    request[0].set_customername("danyang");
    request[0].add_hotelid();
    request[0].set_hotelid(0, kHotelId);
    request[0].set_indate("2022-09-01");
    request[0].set_outdate("2022-09-02");
    request[0].set_roomnumber(128);

    request[1].set_customername("matt");
    request[1].add_hotelid();
    request[1].set_hotelid(0, kHotelId);
    request[1].set_indate("2022-09-02");
    request[1].set_outdate("2022-09-03");
    request[1].set_roomnumber(129);

    req_size[0] = request[0].ByteSizeLong();
    req_size[1] = request[1].ByteSizeLong();

    init = true;
  }
  // 1% danyang, 99% matt
  int index = 1 - (int)(fast_rand() % 100 < 1);
  return {request[index], req_size[index]};
}

int HotelReservationClientApp::Run() {
  Init();

  int req_cnt = 0;
  for (int i = 0; i < opts_.concurrency; i++) {
    auto [request, size] = GenerateWorkload();
    new AsyncClientCall(stub_, &cq_, request, size);
    req_cnt++;
  }

  void* tag;
  bool ok = false;
  long count = 0, overlimit = 0;
  size_t rx_size = 0;
  bool timeout = false;
  int time_period = 1000, time_cnt = 0;
  auto time_dura = std::chrono::seconds(static_cast<long>(opts_.time_duration_sec));
  auto start = std::chrono::high_resolution_clock::now();

  Meter meter = Meter(1000, "meter", 1);
  while (req_cnt > 0 && cq_.Next(&tag, &ok)) {
    AsyncClientCall* call = static_cast<AsyncClientCall*>(tag);
    if (call->status.ok()) {
      count++;
      rx_size += call->result.ByteSizeLong();
      meter.AddQps(call->size, 1);
    } else {
      overlimit++;
      meter.AddQps(0, 0);
    }

    delete call;
    req_cnt--;

    time_cnt += 1;
    if (GPR_UNLIKELY(time_cnt >= time_period)) {
      // Do not check time every iteration.
      time_cnt = 0;
      auto now = std::chrono::high_resolution_clock::now();
      timeout = (now - start > time_dura);
    }

    if (GPR_LIKELY(!timeout)) {
      auto [request, size] = GenerateWorkload();
      new AsyncClientCall(stub_, &cq_, request, size);
      req_cnt++;
    }
  }

  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> seconds = end - start;
  double duration = seconds.count();

  printf("Overlimit: %ld\n", overlimit);
  printf("tx_gbps\t rx_gbps\t tx_rps\t rx_rps\n");
  double tx_gbps = 8.0 * count * opts_.data_size / duration / 1e9;
  double rx_gbps = 8.0 * rx_size / duration / 1e9;
  double tx_rps = count / duration;
  double rx_rps = tx_rps;
  printf("%.6lf\t %.6lf\t %.2lf\t %.2lf\n", tx_gbps, rx_gbps, tx_rps, rx_rps);

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
  // ServerBuilder builder;
  std::unique_ptr<ServerBuilder> builder;
  if (opts_.xds) {
    builder = std::unique_ptr<ServerBuilder>(new ::grpc::XdsServerBuilder());
  } else {
    builder = std::unique_ptr<ServerBuilder>(new ServerBuilder());
  }

  int hardware_concurreny = std::thread::hardware_concurrency();
  int new_max_threads = prism::GetEnvOrDefault<int>("RPC_BENCH_GRPC_MAX_THREAD", hardware_concurreny);
  RPC_LOG(DEBUG) << "gRPC thread pool is set to use " << new_max_threads << " threads";

  ResourceQuota quota;
  quota.SetMaxThreads(new_max_threads);
  builder->SetResourceQuota(quota);
  builder->SetMaxMessageSize(std::numeric_limits<int>::max());
  builder->AddListeningPort(bind_uri, ::grpc::InsecureServerCredentials());
  builder->RegisterService(&service_);


  cq_ = builder->AddCompletionQueue();
  server_ = std::unique_ptr<Server>(builder->BuildAndStart());
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
