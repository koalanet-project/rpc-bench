#include "grpc/grpc_tput_app.h"

#include "logging.h"

namespace rpc_bench {
namespace grpc {

void GrpcTputClientApp::Init() {
  std::string bind_address = opts_.host.value() + ":" + std::to_string(opts_.port);
  auto channel = ::grpc::CreateChannel(bind_address, ::grpc::InsecureChannelCredentials());
  stub_ = LatTputService::NewStub(channel);
}

int GrpcTputClientApp::Run() {
  Init();

  Data data;
  data.set_data(std::string(opts_.data_size, 'a'));
  int req_cnt = 0;
  for (int i = 0; i < kDefaultConcurrency; i++) {
    new AsyncClientCall(stub_, &cq_, data);
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

  while (req_cnt > 0 && cq_.Next(&tag, &ok)) {
    AsyncClientCall* call = static_cast<AsyncClientCall*>(tag);
    if (call->status.ok()) {
      count++;
      rx_size += call->ack.ByteSizeLong();
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
      new AsyncClientCall(stub_, &cq_, data);
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

}  // namespace grpc
}  // namespace rpc_bench