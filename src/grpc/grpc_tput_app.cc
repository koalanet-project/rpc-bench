#include "grpc/grpc_tput_app.h"

#include "logging.h"

namespace rpc_bench {
namespace grpc {

void GrpcTputClientApp::Init() {
  std::string bind_address = opts_.host.value() + ":" + std::to_string(opts_.port);
  auto channel = ::grpc::CreateChannel(bind_address, ::grpc::InsecureChannelCredentials());
  stub_ = LatTputService::NewStub(channel);
  call_per_req_ = 0;
}

int GrpcTputClientApp::Run() {
  Init();

  void* tag;
  bool ok = false;
  Data data;
  data.set_data(std::string(opts_.data_size, 'a'));

  int req_cnt = 0;
  for (int i = 0; i < kDefaultConcurrency; i++) {
    new AsyncClientCall(stub_, &cq_);
    req_cnt++;
  }

  long rx_cnt = 0, tx_cnt = 0, overlimit = 0;
  bool time_out = false;
  int time_period = 1000, time_cnt = 0;
  auto time_dura = std::chrono::microseconds(static_cast<long>(opts_.time_duration_sec * 1e6));
  auto start = std::chrono::high_resolution_clock::now();

  while (req_cnt > 0 && cq_.Next(&tag, &ok)) {
    AsyncClientCall* call = static_cast<AsyncClientCall*>(tag);
    // GPR_ASSERT(ok);
    if (GPR_UNLIKELY(!ok)) {
      overlimit += 1;
      delete call;
      req_cnt--;
      if (GPR_LIKELY(!time_out)) {
        call = new AsyncClientCall(stub_, &cq_);
        req_cnt++;
      }
      continue;
    }

    switch (call->call_status) {
      case AsyncClientCall::WRITE:
        call->stream->Write(data, (void*)call);
        tx_cnt++;
        call->count++;
        call->call_status = AsyncClientCall::READ;
        break;
      case AsyncClientCall::READ:
        call->stream->Read(&call->ack, (void*)call);
        rx_cnt++;
        if ((call_per_req_ > 0 && call->count >= call_per_req_) || time_out)
          call->call_status = AsyncClientCall::WRITEDONE;
        else
          call->call_status = AsyncClientCall::WRITE;
        break;
      case AsyncClientCall::WRITEDONE:
        call->stream->WritesDone((void*)call);
        call->call_status = AsyncClientCall::FINISH;
        break;
      case AsyncClientCall::FINISH:
        call->stream->Finish(&call->status, (void*)call);
        call->call_status = AsyncClientCall::CLOSED;
        break;
      case AsyncClientCall::CLOSED:
        delete call;
        req_cnt--;
        if (GPR_LIKELY(!time_out)) {
          new AsyncClientCall(stub_, &cq_);
          req_cnt++;
        }
        break;
    }

    time_cnt += 1;
    if (time_cnt >= time_period) {
      time_cnt = 0;
      auto now = std::chrono::high_resolution_clock::now();
      time_out = (now - start > time_dura);
    }
  }

  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> seconds = end - start;
  double duration = seconds.count();

  printf("rx_gbps,\t tx_gbps,\t rx_rps,\t tx_rps\n");
  double rx_gbps = 8.0 * rx_cnt * 4 / duration / 1e9;
  double tx_gbps = 8.0 * tx_cnt * opts_.data_size / duration / 1e9;
  double rx_rps = rx_cnt / duration;
  double tx_rps = rx_cnt / duration;
  printf("%.6lf,\t %.6lf,\t %.1lf,\t %.1lf\n", rx_gbps, tx_gbps, rx_rps, tx_rps);

  return 0;
}

}  // namespace grpc
}  // namespace rpc_bench