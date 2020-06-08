#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "grpc/grpc_bw_app.h"
#include "command_opts.h"
#include "bw_app.h"

namespace rpc_bench {
namespace grpc {

// GrpcBwClientApp
void GrpcBwClientApp::Init() {
  std::string remote_uri = opts_.host.value() + std::to_string(opts_.port);
  RPC_LOG(DEBUG) << "gRPC client is connecting to remote_uri: " << remote_uri;
  stub_ = std::make_unique<BwService::Stub>(::grpc::CreateChannel(remote_uri, ::grpc::InsecureChannelCredentials()));
}

void GrpcBwClientApp::IssueBwReq(const BwMessage& bw_msg, BwAck *bw_ack) {
  PbBwMessage request;
  PackPbBwMessage(bw_msg, &request);
  PbBwAck reply;
  // TODO(cjr): figure out whether it really needs a per call ClientContext
  ClientContext context;
  Status status = stub_->Request(&context, request, &reply);
  if (status.ok()) {
    bw_ack->success = true;
    UnpackPbBwHeader(&bw_ack->header, reply.header());
  } else {
    RPC_LOG(ERROR) << status.error_code() << ": " << status.error_message();
    bw_ack->success = false;
  }
}

void PackPbBwMessage(const BwMessage& bw_msg, PbBwMessage* pb_bw_msg) {
  PbBwHeader pb_header;
  const BwHeader& header = bw_msg.header;
  PackPbBwHeader(header, &pb_header);

  // TODO(cjr): this may have some problems
  pb_bw_msg->set_allocated_header(&pb_header);
  pb_bw_msg->set_data(bw_msg.data);
}

void UnpackPbBwHeader(BwHeader* bw_header, const PbBwHeader& pb_bw_header) {
  bw_header->field1 = pb_bw_header.field1();
  bw_header->field2 = pb_bw_header.field2();
  bw_header->field3 = pb_bw_header.field3();
}

void PackPbBwHeader(const BwHeader bw_header, PbBwHeader *pb_bw_header) {
  pb_bw_header->set_field1(bw_header.field1);
  pb_bw_header->set_field2(bw_header.field2);
  pb_bw_header->set_field3(bw_header.field3);
}

// grpc server handler
Status BwServiceImpl::Request(ServerContext* context, const PbBwMessage* request,
                              PbBwAck* reply) {
  // TODO(cjr): check if this is OK
  BwHeader header;
  UnpackPbBwHeader(&header, request->header());
  PbBwHeader pb_header;
  PackPbBwHeader(header, &pb_header);
  reply->set_allocated_header(&pb_header);
  return Status::OK;
}

// GrpcBwServerApp
void GrpcBwServerApp::Init() {
  std::string bind_uri = "0.0.0.0" + std::to_string(opts_.port);

  ::grpc::EnableDefaultHealthCheckService(true);
  ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
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
