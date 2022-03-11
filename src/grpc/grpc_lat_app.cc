#include "grpc/grpc_lat_app.h"

#include "logging.h"

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
  std::vector<double> latencies;
  bool time_out = false;
  auto time_dura = std::chrono::microseconds(static_cast<long>(opts_.time_duration_sec * 1e6));
  auto start = std::chrono::high_resolution_clock::now(), last = start;

  while (!time_out) {
    AsyncClientCall* call = new AsyncClientCall(stub_, &cq_, data);

    void* tag;
    bool ok = false;
    GPR_ASSERT(cq_.Next(&tag, &ok));

    if (call->status.ok()) {
      auto now = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> diff = now - last;
      latencies.push_back(1e6 * diff.count());
      time_out = (now - start > time_dura);
      last = std::chrono::high_resolution_clock::now();  // latencies.push_back may take some time
    } else
      overlimit++;
  }

  std::sort(latencies.begin(), latencies.end());
  double sum = 0;
  for (double latency : latencies) sum += latency;
  size_t len = latencies.size();
  printf("overlimit: %ld\n", overlimit);
  printf(
      "%zu\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t "
      "[%zu samples]\n",
      opts_.data_size, sum / len, latencies[(int)(0.5 * len)], latencies[(int)(0.05 * len)],
      latencies[(int)(0.99 * len)], latencies[(int)(0.999 * len)], latencies[(int)(0.9999 * len)],
      latencies[(int)(0.99999 * len)], latencies[(int)(0.999999 * len)], latencies[len - 1], len);

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

void GrpcLatServerApp::Init() {}

int GrpcLatServerApp::Run() {
  std::string bind_address = "0.0.0.0:" + std::to_string(opts_.port);
  ServerBuilder builder;
  builder.AddListeningPort(bind_address, ::grpc::InsecureServerCredentials());

  builder.RegisterService(&service_);
  cq_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();
  printf("Async server listening on %s\n", bind_address.c_str());

  new AsyncServerCall(&service_, cq_.get(), opts_.data_size);
  void* tag;
  bool ok;
  while (true) {
    GPR_ASSERT(cq_->Next(&tag, &ok));
    static_cast<AsyncServerCall*>(tag)->Proceed();
  }
}

}  // namespace grpc
}  // namespace rpc_bench