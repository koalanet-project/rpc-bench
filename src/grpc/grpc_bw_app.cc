#include "grpc/grpc_bw_app.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

#include <limits>
#include <thread>

#include "bw_app.h"
#include "command_opts.h"
#include "prism/utils.h"
#include "logging.h"

namespace rpc_bench {
namespace grpc {

// GrpcBwClientApp
void GrpcBwClientApp::Init() {
  std::string remote_uri = opts_.host.value() + ":" + std::to_string(opts_.port);
  RPC_LOG(DEBUG) << "gRPC client is connecting to remote_uri: " << remote_uri;
  stub_ = std::make_unique<BwService::Stub>(
      ::grpc::CreateChannel(remote_uri, ::grpc::InsecureChannelCredentials()));
}

void GrpcBwClientApp::PushData(const BwMessage& bw_msg, BwAck* bw_ack) {
  PbBwMessage request;
  PackPbBwMessage(bw_msg, &request);
  RPC_LOG(TRACE) << "request header size: " << request.header().ByteSizeLong();
  RPC_LOG(TRACE) << "request size: " << request.ByteSizeLong();
  PbBwAck reply;
  // TODO(cjr): figure out whether it really needs a per call ClientContext
  ClientContext context;
  Status status = stub_->PushData(&context, request, &reply);
  if (status.ok()) {
    bw_ack->success = true;
    UnpackPbBwHeader(&bw_ack->header, reply.header());
  } else {
    RPC_LOG(DEBUG) << status.error_code() << ": " << status.error_message();
    bw_ack->success = false;
  }
}

// grpc server handler
Status BwServiceImpl::PushData(ServerContext* context, const PbBwMessage* request, PbBwAck* reply) {
  // TODO(cjr): check if this is OK
  BwHeader header;
  UnpackPbBwHeader(&header, request->header());
  PackPbBwHeader(header, reply->mutable_header());
  meter_.AddBytes(request->ByteSizeLong());
  return Status::OK;
}

Status BwServiceImpl::StopServer(ServerContext* context, const StopRequest* req, StopResponse* res) {
  return Status::OK;
}

// GrpcBwServerApp
void GrpcBwServerApp::Init() {
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

  server_ = std::unique_ptr<Server>(builder.BuildAndStart());
  RPC_LOG(DEBUG) << "gRPC server is listening on uri: " << bind_uri;
}

int GrpcBwServerApp::Run() {
  Init();
  server_->Wait();
  return 0;
}

}  // namespace grpc
}  // namespace rpc_bench
