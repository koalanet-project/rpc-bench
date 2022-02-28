#include "zeromq/zeromq_bw_app.h"

#include <limits>
#include <thread>

#include "bw_app.h"
#include "command_opts.h"
#include "logging.h"
#include "prism/utils.h"
#include "protos/bw_app.pb.h"

namespace rpc_bench {
namespace zeromq {

inline void FreeData(void* data, void* hint) {
  if (hint != nullptr) {
    delete[] static_cast<char*>(data);
  }
}

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
  // use zero copy push data
  return PushDataZero(bw_msg, bw_ack);
}

void ZeromqBwClientApp::PushDataNaive(const BwMessage& bw_msg, BwAck* bw_ack) {
  bw_app::PbBwHeader pb_header;
  size_t header_size = pb_header.ByteSizeLong();
  size_t data_size = bw_msg.data.size();

  // write message data
  RPC_CHECK_EQ(data_size, zmq_send(zmq_requester_, bw_msg.data.data(), data_size, 0));

  // receive and de-serialize
  RPC_CHECK_EQ(header_size, zmq_recv(zmq_requester_, &recv_buffer_[0], header_size, 0));
  bw_ack->success = true;
}

void ZeromqBwClientApp::PushDataZero(const BwMessage& bw_msg, BwAck* bw_ack) {
  bw_app::PbBwHeader pb_header;
  size_t header_size = pb_header.ByteSizeLong();
  size_t data_size = bw_msg.data.size();

  PackPbBwHeader(bw_msg.header, &pb_header);
  char header_buf[100];
  pb_header.SerializeToArray(header_buf, 100);

  // send header zero copy
  int tag = ZMQ_SNDMORE;
  zmq_msg_t header_msg;
  zmq_msg_init_data(&header_msg, header_buf, header_size, FreeData, nullptr);
  while (true) {
    if (zmq_msg_send(&header_msg, zmq_requester_, tag) == (int)header_size) break;
    RPC_LOG_IF(ERROR, errno != EINTR) << zmq_strerror(errno);
  }
  zmq_msg_close(&header_msg);

  // send data zero copy
  zmq_msg_t data_msg;
  zmq_msg_init_data(&data_msg, const_cast<char*>(bw_msg.data.data()), bw_msg.data.size(), FreeData,
                    nullptr);
  tag = ZMQ_DONTWAIT;
  while (true) {
    if (zmq_msg_send(&data_msg, zmq_requester_, tag) == (int)data_size) break;
    RPC_LOG_IF(ERROR, errno != EINTR)
        << "failed to send message, errno: " << errno << " " << zmq_strerror(errno);
  }
  zmq_msg_close(&data_msg);

  // receive reply
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

  // use zmq multi-part aware receive
  return RunZero();
}

int ZeromqBwServerApp::RunNaive() {
  char recv_buffer[1048576];
  while (true) {
    int bytes_recv = zmq_recv(zmq_responder_, recv_buffer, 1048576, 0);
    if (bytes_recv > 0) meter_.AddBytes(bytes_recv);
    bw_app::PbBwHeader pb_header;
    size_t header_size = pb_header.ByteSizeLong();
    // for now, just send some dummy reply
    RPC_CHECK_EQ(header_size, zmq_send(zmq_responder_, &recv_buffer[0], header_size, 0));
  }
}

int ZeromqBwServerApp::RunZero() {
  bw_app::PbBwHeader pb_header;
  size_t header_size = pb_header.ByteSizeLong();
  while (true) {
    zmq_msg_t zmsg;
    RPC_CHECK_EQ(0, zmq_msg_init(&zmsg)) << zmq_strerror(errno);
    while (true) {
      int bytes_recv = zmq_msg_recv(&zmsg, zmq_responder_, 0);
      if (bytes_recv > 0) meter_.AddBytes(bytes_recv);
      if (!zmq_msg_more(&zmsg)) break;
    }

    zmq_msg_t reply_zmsg;
    // send some dummy reply, use the received data
    zmq_msg_init_data(&reply_zmsg, zmq_msg_data(&zmsg), header_size, FreeData, nullptr);
    while (true) {
      if (zmq_msg_send(&reply_zmsg, zmq_responder_, ZMQ_DONTWAIT) == (int)header_size) break;
      RPC_LOG_IF(ERROR, errno != EINTR) << zmq_strerror(errno);
    }
    zmq_msg_close(&reply_zmsg);
    zmq_msg_close(&zmsg);
  }

  return 0;
}

}  // namespace zeromq
}  // namespace rpc_bench
