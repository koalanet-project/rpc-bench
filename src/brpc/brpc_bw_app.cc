#include "brpc_bw_app.h"
#include "brpc/closure_guard.h"
#include "brpc/controller.h"
#include "brpc/server.h"
#include "bw_app.h"
#include "logging.h"

namespace rpc_bench {
namespace brpc {

void BrpcBwClientApp::Init() {
  ::brpc::ChannelOptions options;
  options.protocol = "baidu_std";
  options.connection_type = "single";  // Available values: single, pooled, short
  options.timeout_ms = 100;
  options.max_retry = 3;
  std::string remote_uri = opts_.host.value() + ":" + std::to_string(opts_.port);
  RPC_LOG(DEBUG) << "brpc client is connecting to remote_uri: " << remote_uri;
  std::string load_balancer = "";
  if (channel_.Init(remote_uri.c_str(), load_balancer.c_str(), &options) != 0) {
    RPC_LOG(FATAL) << "Failed to initialize channel";
  }

  stub_ = std::make_unique<BwServiceBrpc_Stub>(&channel_);
  RPC_LOG(DEBUG) << "brpc client is connected to remote_uri: " << remote_uri;
}

void BrpcBwClientApp::PushData(const BwMessage& bw_msg, BwAck* bw_ack) {
  if (::brpc::IsAskedToQuit()) {
    bw_ack->success = false;
    return;
  }

  ::brpc::Controller cntl;
  PbBwMessage pb_bw_msg;
  PackPbBwHeader(bw_msg.header, pb_bw_msg.mutable_header());
  cntl.set_log_id(log_id_++);
  cntl.request_attachment().append(bw_msg.data);

  PbBwAck pb_bw_ack;
  stub_->PushData(&cntl, &pb_bw_msg, &pb_bw_ack, nullptr);
  if (!cntl.Failed()) {
    bw_ack->success = true;
    UnpackPbBwHeader(&bw_ack->header, pb_bw_ack.header());
  } else {
    RPC_LOG(ERROR) << cntl.ErrorCode() << ": " << cntl.ErrorText();
    bw_ack->success = false;
  }
}

void BwServiceImpl::PushData(RpcController* cntl_base, const PbBwMessage* request, PbBwAck* reply,
                             Closure* done) {
  ::brpc::ClosureGuard done_guard(done);
  ::brpc::Controller* cntl = static_cast<::brpc::Controller*>(cntl_base);
  BwHeader header;
  UnpackPbBwHeader(&header, request->header());
  PackPbBwHeader(header, reply->mutable_header());
  size_t request_size = request->ByteSizeLong() + cntl->request_attachment().size();
  meter_.AddBytes(request_size);
}

void BwServiceImpl::StopServer(RpcController* cntl_base, const StopRequest* req, StopResponse* res,
                               Closure* done) {
  RPC_UNIMPLEMENTED
}

void BrpcBwServerApp::Init() {
  if (server_.AddService(&service_, ::brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    RPC_LOG(FATAL) << "Failed to add service";
  }
  ::brpc::ServerOptions options;
  if (server_.Start(opts_.port, &options) != 0) {
    RPC_LOG(FATAL) << "Failed to start BwServer";
  }
}

int BrpcBwServerApp::Run() {
  Init();
  server_.RunUntilAskedToQuit();
  return 0;
}

}  // namespace brpc
}  // namespace rpc_bench
