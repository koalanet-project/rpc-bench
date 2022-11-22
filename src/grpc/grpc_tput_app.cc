#include "grpc/grpc_tput_app.h"

#include <string>
#include <thread>
#include <vector>

#include "logging.h"
#include "meter.h"

namespace rpc_bench {
namespace grpc {

void GrpcTputClientApp::Init() {}

void GrpcTputClientApp::Job(int thread_id) {
  std::string bind_address = opts_.host.value() + ":" + std::to_string(opts_.port);
  ::grpc::ChannelArguments args;
  args.SetInt(GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL, 1);
  auto channel =
      ::grpc::CreateCustomChannel(bind_address, ::grpc::InsecureChannelCredentials(), args);
  std::unique_ptr<LatTputService::Stub> stub = LatTputService::NewStub(channel);

  Data data;
  data.set_data(std::string(opts_.data_size, 'a'));
  int req_cnt = 0;
  for (int i = 0; i < opts_.concurrency; i++) {
    new AsyncClientCall(stub, cq_[thread_id].get(), data);
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

  Meter meter = Meter(1000, ("meter" + std::to_string(thread_id)).c_str(), 1);
  while (req_cnt > 0 && cq_[thread_id]->Next(&tag, &ok)) {
    AsyncClientCall* call = static_cast<AsyncClientCall*>(tag);
    if (call->status.ok()) {
      count++;
      rx_size += call->ack.ByteSizeLong();
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
      new AsyncClientCall(stub, cq_[thread_id].get(), data);
      req_cnt++;
    }
  }

  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> seconds = end - start;
  double duration = seconds.count();

  double tx_gbps = 8.0 * count * opts_.data_size / duration / 1e9;
  double rx_gbps = 8.0 * rx_size / duration / 1e9;
  double tx_rps = count / duration;
  double rx_rps = tx_rps;
  std::cout << thread_id << " "
                << "Overlimit: " << overlimit;
  std::cout << thread_id << " "
                << prism::FormatString(
                       "tx_gbps: %.3lf\t rx_gbps: %.3lf\t tx_rps: %.3lf\t rx_rps:%.3lf\n", tx_gbps,
                       rx_gbps, tx_rps, rx_rps);
}

int GrpcTputClientApp::Run() {
  cq_.reserve(opts_.thread);
  for (int i = 0; i < opts_.thread; i++) {
    cq_.emplace_back(std::make_unique<CompletionQueue>());
  }

  std::vector<std::thread> threads;
  threads.reserve(opts_.thread);
  for (int i = 0; i < opts_.thread; i++) {
    threads.emplace_back(&GrpcTputClientApp::Job, this, i);
  }

  for (int i = 0; i < opts_.thread; i++) {
    threads[i].join();
  }
  return 0;
}

}  // namespace grpc
}  // namespace rpc_bench
