#include "socket/bw_server_endpoint.h"

#include <sys/socket.h>

#include <memory>

#include "logging.h"
#include "prism/logging.h"
#include "socket/poll.h"
#include "socket/socket.h"
#include "socket/socket_bw_app.h"

namespace rpc_bench {
namespace socket {

BwServerEndpoint::BwServerEndpoint(TcpSocket new_sock, SocketBwServerApp* bw_server_app)
    : sock_{new_sock},
      app_{bw_server_app},
      interest_{Interest::READABLE | Interest::WRITABLE},
      state_{State::NEW_RPC} {}

BwServerEndpoint::~BwServerEndpoint() {
  if (!sock_.IsClosed()) {
    sock_.Close();
  }
}

void BwServerEndpoint::OnAccepted() {
  GetPeerAddr();
  sock_.SetNonBlock(true);
  app_->poll().registry().Register(sock_, Token(reinterpret_cast<uintptr_t>(this)), interest_);
  RPC_LOG(INFO) << "accept connection from: " << sock_addr_.AddrStr();
}

void BwServerEndpoint::GetPeerAddr() {
  struct sockaddr_storage storage;
  struct sockaddr* sockaddr = reinterpret_cast<struct sockaddr*>(&storage);
  socklen_t len = sizeof(struct sockaddr_storage);
  PCHECK(!getpeername(sock_, sockaddr, &len));
  sock_addr_ = SockAddr(sockaddr, len);
}

void BwServerEndpoint::OnError() {
  RPC_LOG(ERROR) << "Socket error: " << sock_.GetSockError()
                 << ", remote uri: " << sock_addr_.AddrStr();
  app_->poll().registry().Deregister(sock_);
  sock_.Close();
}

void BwServerEndpoint::OnRecvReady() {
  // TODO(cjr): rewrite this in a coroutine or a generator way
  switch (state_) {
    case State::NEW_RPC: {
      // read meta header
      if (!ReceiveMeta()) break;
      // receive header
      if (!ReceiveHeader()) break;
      // receive data
      if (!ReceiveData()) break;
      // reply
      if (!Reply()) break;
    } break;
    case State::META_RECVED: {
      if (!ReceiveHeader()) break;
      if (!ReceiveData()) break;
      if (!Reply()) break;
    } break;
    case State::HEADER_RECVED: {
      if (!ReceiveData()) break;
      if (!Reply()) break;
    } break;
    case State::DATA_RECVED: {
      if (!Reply()) break;
    } break;
    case State::REPLYING: {
      // do nothing
    } break;
    default: {
      RPC_LOG(FATAL) << "unexpected state: " << static_cast<int>(state_);
    }
  }
}

bool BwServerEndpoint::ReceiveMeta() {
  uint32_t meta_buffer[2];
  // force blocking receive here
  if (sock_.RecvAll(meta_buffer, sizeof(meta_buffer)) < sizeof(meta_buffer)) {
    return false;
  }
  uint32_t header_size = meta_buffer[0];
  uint32_t data_size = meta_buffer[1];
  // allocate buffer
  if (!header_buffer_owner_ ||
      (header_buffer_owner_ && header_buffer_owner_->msg_length() < header_size)) {
    header_buffer_owner_ = std::make_unique<Buffer>(header_size);
    header_buffer_owner_->set_msg_length(header_size);
  }
  if (!data_buffer_owner_ ||
      (data_buffer_owner_ && data_buffer_owner_->msg_length() < data_size)) {
    data_buffer_owner_ = std::make_unique<Buffer>(data_size);
    data_buffer_owner_->set_msg_length(data_size);
  }
  header_buffer_ = std::make_unique<Buffer>(header_buffer_owner_.get());
  data_buffer_ = std::make_unique<Buffer>(data_buffer_owner_.get());
  state_ = State::META_RECVED;
  DLOG(TRACE) << "header_size: " << header_size << ", data_size: " << data_size;
  return true;
}

bool BwServerEndpoint::ReceiveHeader() {
  ssize_t nbytes = sock_.Recv(header_buffer_->GetRemainBuffer(), header_buffer_->GetRemainSize());
  DLOG(TRACE) << "ReceiveHeader: " << nbytes;
  if (nbytes == -1) {
    PCHECK(sock_.LastErrorWouldBlock());
  } else {
    header_buffer_->MarkHandled(nbytes);
    if (header_buffer_->IsClear()) {
      state_ = State::HEADER_RECVED;
    }
  }
  return header_buffer_->IsClear();
}

bool BwServerEndpoint::ReceiveData() {
  ssize_t nbytes = sock_.Recv(data_buffer_->GetRemainBuffer(), data_buffer_->GetRemainSize());
  DLOG(TRACE) << "ReceiveData: " << nbytes;
  if (nbytes == -1) {
    PCHECK(sock_.LastErrorWouldBlock());
  } else {
    data_buffer_->MarkHandled(nbytes);
    if (data_buffer_->IsClear()) {
      state_ = State::DATA_RECVED;
      // reuse the header_buffer_ for replying, only to create a view on it
      reply_buffer_ = std::make_unique<Buffer>(header_buffer_.get());
    }
  }
  return data_buffer_->IsClear();
}

bool BwServerEndpoint::Reply() {
  // TODO(cjr): this would never block but may encounter oom error,
  // we will handle the error after doing some benchmarks
  DLOG(TRACE) << "Reply";
  tx_queue_.push(std::move(reply_buffer_));
  state_ = State::REPLYING;
  return true;
}

void BwServerEndpoint::OnSendReady() {
  // TODO(cjr): think of whether to fair share among tx data
  while (!tx_queue_.empty()) {
    auto &buffer = tx_queue_.front();
    ssize_t nbytes = sock_.Send(buffer->GetRemainBuffer(), buffer->GetRemainSize());
    if (nbytes == -1) {
      PCHECK(sock_.LastErrorWouldBlock());
      break;
    }
    buffer->MarkHandled(nbytes);
    if (buffer->IsClear()) {
      state_ = State::NEW_RPC;  // RPC completed
      tx_queue_.pop();
      // std::mem::drop(buffer), be aware that the buffer reference is
      // out-dated
    }
  }
}

void BwServerEndpoint::HandleReceivedData(BufferPtr buffer) {
  // never used
}

}  // namespace socket
}  // namespace rpc_bench
