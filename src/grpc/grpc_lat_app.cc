#include "grpc/grpc_lat_app.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

// xDS
#include <grpcpp/xds_server_builder.h>

#include <limits>
#include <thread>

#include "logging.h"
#include "prism/utils.h"

namespace rpc_bench {
namespace grpc {

void GrpcLatClientApp::Init() {
  std::string bind_address = opts_.host.value() + ":" + std::to_string(opts_.port);
  auto channel = ::grpc::CreateChannel(bind_address, ::grpc::InsecureChannelCredentials());
  stub_ = LatTputService::NewStub(channel);
}

int GrpcLatClientApp::Run() {
  Init();

  Data data;
  data.set_data(std::string(opts_.data_size, 'a'));

  long overlimit = 0;
  bool timeout = false;
  auto time_dura = std::chrono::microseconds(static_cast<long>(opts_.time_duration_sec * 1e6));
  auto start = std::chrono::high_resolution_clock::now(), last = start;

  while (!timeout) {
    AsyncClientCall* call = new AsyncClientCall(stub_, &cq_, data);

    void* tag;
    bool ok = false;
    GPR_ASSERT(cq_.Next(&tag, &ok));

    if (call->status.ok()) {
      auto now = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> diff = now - last;
      latencies.push_back(1e6 * diff.count());
      timeout = (now - start > time_dura);
      last = std::chrono::high_resolution_clock::now();  // latencies.push_back may take some time
    } else
      overlimit++;
  }

  printf("Overlimit: %ld\n", overlimit);
  print();
  return 0;
}

void GrpcLatServerApp::AsyncServerCall::Proceed() {
  switch (status_) {
    case CREATE:
      status_ = PROCESS;
      service_->RequestSendData(&ctx_, &data_, &responder_, cq_, cq_, this);
      break;

    case PROCESS:
      new AsyncServerCall(service_, cq_, data_size_);
      status_ = FINISH;
      responder_.Finish(ack_, Status::OK, this);
      break;

    case FINISH:
      delete this;
      break;
  }
}

void GrpcLatServerApp::Job(int thread_id) {
  new AsyncServerCall(&service_, cq_[thread_id].get(), opts_.data_size);
  void* tag;
  bool ok;
  while (true) {
    GPR_ASSERT(cq_[thread_id]->Next(&tag, &ok));
    static_cast<AsyncServerCall*>(tag)->Proceed();
  }
}

void GrpcLatServerApp::Init() {}

int GrpcLatServerApp::Run() {
  std::string bind_address = "0.0.0.0:" + std::to_string(opts_.port);

  int hardware_concurreny = std::thread::hardware_concurrency();
  int new_max_threads =
      prism::GetEnvOrDefault<int>("RPC_BENCH_GRPC_MAX_THREAD", hardware_concurreny);
  RPC_LOG(DEBUG) << "gRPC thread pool is set to use " << new_max_threads << " threads";

  // ServerBuilder builder;
  ::grpc::XdsServerBuilder builder;

  //   ::grpc::ResourceQuota quota;
  //   quota.SetMaxThreads(new_max_threads);
  //   builder.SetResourceQuota(quota);
  builder.SetMaxMessageSize(std::numeric_limits<int>::max());
  builder.AddListeningPort(bind_address, ::grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);

  cq_.reserve(opts_.thread);
  for (int i = 0; i < opts_.thread; i++) {
    cq_.emplace_back(builder.AddCompletionQueue());
  }
  server_ = builder.BuildAndStart();
  printf("Async server listening on %s\n", bind_address.c_str());

  std::vector<std::thread> threads;
  for (int i = 0; i < opts_.thread; i++) {
    threads.emplace_back(&GrpcLatServerApp::Job, this, i);
  }

  for (int i = 0; i < opts_.thread; i++) {
    threads[i].join();
  }

  return 0;
}

}  // namespace grpc
}  // namespace rpc_bench
