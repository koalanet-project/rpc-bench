#include "zeromq/zeromq_bw_app.h"

#include <limits>
#include <thread>

#include "bw_app.h"
#include "command_opts.h"
#include "prism/utils.h"
#include "logging.h"

namespace rpc_bench {
namespace zeromq {

// ZeromqBwClientApp
void ZeromqBwClientApp::Init() {
  std::string remote_uri = opts_.host.value() + ":" + std::to_string(opts_.port);
  RPC_LOG(DEBUG) << "zeromq client is connecting to remote_url: " << remote_uri;
  zmq_context_ = zmq_ctx_new();
  zmq_requester_ = zmq_socket(zmq_context_, ZMQ_REQ);
  std::string zmq_addr = "tcp://" + remote_uri;
  zmq_connect(zmq_requester_, zmq_addr.c_str());
}

void ZeromqBwClientApp::PushData(const BwMessage& bw_msg, BwAck* bw_ack) {
  bw_app::PbBwHeader pb_header;
  size_t header_size = pb_header.ByteSizeLong();
  size_t data_size = bw_msg.data.size();
  // write message data

  // whether to do copy here only affect the cpu utilization, it has negligible
  // influence in bandwidth throughput
  RPC_CHECK_EQ(data_size, zmq_send(zmq_requester_, bw_msg.data.data(), data_size, 0));
  // memcpy(send_buffer_.data(), bw_msg.data.data(), data_size);
  // RPC_CHECK_EQ(data_size, sock_.SendAll(send_buffer_.data(), data_size, 0));

  // receive and de-serialize
  RPC_CHECK_EQ(header_size, zmq_recv(zmq_requester_, &recv_buffer_[0], header_size, 0));
  bw_ack->success = true;
}

// ZeromqServerApp
void ZeromqBwServerApp::Init() {
  std::string bind_uri = "0.0.0.0:" + std::to_string(opts_.port);
  zmq_context_ = zmq_ctx_new();
  zmq_responder_ = zmq_socket(zmq_context_, ZMQ_REP);
  std::string zmq_addr = "tcp://" + bind_uri;
  RPC_CHECK_EQ(0, zmq_bind(zmq_responder_, zmq_addr.c_str()));
}

int ZeromqBwServerApp::Run() {
  Init();
  char recv_buffer[1048576];
  while (true) {
    int bytes_recv = zmq_recv(zmq_responder_, recv_buffer, 1048576, 0);
    bw_app::PbBwHeader pb_header;
    size_t header_size = pb_header.ByteSizeLong();
    // for now, just send some dummy reply
    RPC_CHECK_EQ(header_size, zmq_send(zmq_responder_, &recv_buffer[0], header_size, 0));
  }
}

}  // namespace zeromq
}  // namespace rpc_bench
