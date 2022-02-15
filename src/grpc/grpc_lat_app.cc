#include "grpc/grpc_lat_app.h"

#include "logging.h"

namespace rpc_bench {
namespace grpc {

void GrpcLatClientApp::Init() {
  std::string bind_address = opts_.host.value() + ":" + std::to_string(opts_.port);
  auto channel = ::grpc::CreateChannel(bind_address, ::grpc::InsecureChannelCredentials());
  stub_ = LatService::NewStub(channel);
}

int GrpcLatClientApp::Run() {
  AsyncClientCall* call = new AsyncClientCall(stub_, &cq_);
  void* tag;
  bool ok = false;

  long tx_cnt = 0, rx_cnt = 0;
  std::string payload = std::string(opts_.data_size, 'a');

  std::vector<double> latencies;
  auto time_dura = std::chrono::microseconds(static_cast<long>(opts_.time_duration_sec * 1e6));
  auto start = std::chrono::high_resolution_clock::now();
  auto last = start;

  while (cq_.Next(&tag, &ok)) {
    GPR_ASSERT(ok);
    call = static_cast<AsyncClientCall*>(tag);

    if (call->finished) break;
    if (call->sendfinished) {
      call->stream->Finish(&call->status, (void*)call);
      continue;
    }

    if (call->writing == true) {
      Data d;
      d.set_data(payload);
      call->writing = false;
      call->stream->Write(d, (void*)call);
      tx_cnt++;
      continue;
    } else {
      call->writing = true;
      call->stream->Read(&call->ack, (void*)call);
      rx_cnt++;
    }

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = now - last;
    latencies.push_back(diff.count());
    if (now - start > time_dura) {
      call->stream->WritesDone((void*)call);
      call->sendfinished = true;
    }
    last = std::chrono::high_resolution_clock::now();  // latencies.push_back may take some time
  }

  if (call->status.ok()) {
    std::sort(latencies.begin(), latencies.end());
    size_t len = latencies.size();
    printf(
        "%zu,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f "
        "[%zu samples]\n",
        opts_.data_size, latencies[(int)(0.5 * len)], latencies[(int)(0.05 * len)],
        latencies[(int)(0.99 * len)], latencies[(int)(0.999 * len)], latencies[(int)(0.9999 * len)],
        latencies[(int)(0.99999 * len)], latencies[(int)(0.999999 * len)], latencies[len - 1], len);
  } else {
    RPC_LOG(ERROR) << call->status.error_code() << ": " << call->status.error_message();
  }

  return 0;
}

void GrpcLatServerApp::AsyncServerCall::Proceed(bool ok) {
  switch (status_) {
    case CREATE:
      status_ = PROCESS;
      service_->RequestSendDataStreamFullDuplex(&ctx_, &stream_, cq_, cq_, this);
      break;

    case PROCESS:
      new AsyncServerCall(service_, cq_, data_size_);
      status_ = READ_COMPLETE;
      stream_.Read(&data_, this);
      break;

    case READ_COMPLETE:
      if (ok) {
        status_ = WRITE_COMPLETE;
        stream_.Write(ack_, this);
      } else {
        status_ = FINISH;
        stream_.Finish(Status::OK, this);
      }
      break;

    case WRITE_COMPLETE:
      if (ok) {
        status_ = READ_COMPLETE;
        stream_.Read(&data_, this);
      } else {
        status_ = FINISH;
        stream_.Finish(Status::OK, this);
      }
      break;

    case FINISH:
      delete this;
      break;
  }
}

void GrpcLatServerApp::Init() {}

int GrpcLatServerApp::Run() {
  std::string bind_address = opts_.host.value() + ":" + std::to_string(opts_.port);
  ServerBuilder builder;
  builder.AddListeningPort(bind_address, ::grpc::InsecureServerCredentials());

  builder.RegisterService(&service_);
  cq_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();
  printf("Async server listening on %s:%d\n", opts_.host.value().c_str(), opts_.port);

  new AsyncServerCall(&service_, cq_.get(), opts_.data_size);
  void* tag;
  bool ok;
  std::string a;
  while (true) {
    GPR_ASSERT(cq_->Next(&tag, &ok));
    static_cast<AsyncServerCall*>(tag)->Proceed(ok);
  }
}

}  // namespace grpc
}  // namespace rpc_bench