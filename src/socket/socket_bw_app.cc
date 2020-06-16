#include "socket/socket_bw_app.h"

#include <sys/socket.h>

#include <chrono>
#include <optional>
#include <queue>
#include <thread>

#include "bw_app.h"
#include "socket/bw_server_endpoint.h"
#include "socket/poll.h"

namespace rpc_bench {
namespace socket {

void SocketBwClientApp::Init() {
  AddrInfo ai(opts_.host.value().c_str(), opts_.port, SOCK_STREAM);
  RPC_LOG(DEBUG) << "socket client is connecting to remote_uri: " << ai.AddrStr();
  sock_.Create(ai);
  while (!sock_.Connect(ai)) {
    RPC_LOG(WARNING) << "connect failed, error code: " << sock_.GetLastError() << ", retrying after 1 second...";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  RPC_LOG(DEBUG) << "connected to: " << ai.AddrStr();
  send_buffer_.resize(kDefaultBufferSize);
  recv_buffer_.resize(kDefaultBufferSize);
}

void SocketBwClientApp::PushData(const BwMessage& bw_msg, BwAck* bw_ack) {
  // serialize and send
  bw_app::PbBwHeader pb_header;
  PackPbBwHeader(bw_msg.header, &pb_header);
  size_t header_size = pb_header.ByteSizeLong();
  size_t data_size = bw_msg.data.size();
  RPC_LOG(TRACE) << "header_size: " << header_size << ", data_size: " << data_size;
  // write meta header
  size_t meta_size = 2 * sizeof(uint32_t);
  *reinterpret_cast<uint32_t*>(send_buffer_.data()) = header_size;
  *reinterpret_cast<uint32_t*>(send_buffer_.data() + sizeof(uint32_t)) = data_size;
  // write message header
  pb_header.SerializeToArray(send_buffer_.data() + meta_size, header_size);
  RPC_CHECK_EQ(header_size + meta_size,
               sock_.SendAll(send_buffer_.data(), header_size + meta_size, 0));
  // write message data
  RPC_CHECK_EQ(data_size, sock_.SendAll(bw_msg.data.data(), data_size, 0));

  // receive and de-serialize
  size_t r = sock_.RecvAll(recv_buffer_.data(), header_size, 0);
  if (r < header_size) {
    RPC_LOG(ERROR) << "only " << r << " ( expected " << header_size
                   << ") bytes received, peer has performed an orderly shutdown";
    bw_ack->success = false;
  } else {
    bw_ack->success = true;
    pb_header.ParseFromArray(recv_buffer_.data(), header_size);
    UnpackPbBwHeader(&bw_ack->header, pb_header);
  }
}

void SocketBwServerApp::HandleNewConnection() {
  TcpSocket new_sock = listener_.Accept();
  auto endpoint = std::make_shared<BwServerEndpoint>(new_sock, this);
  endpoint->OnAccepted();
  endpoints_[endpoint->fd()] = endpoint;
}

void SocketBwServerApp::Init() {
  AddrInfo ai(opts_.port, SOCK_STREAM);
  listener_.Create(ai);
  listener_.SetReuseAddr(true);
  listener_.SetNonBlock(true);
  listener_.Bind(ai);
  listener_.Listen();

  poll_.registry().Register(listener_, Token(listener_.sockfd), Interest::READABLE);

  RPC_LOG(DEBUG) << "socket server is listening on uri: " << ai.AddrStr();
}

int SocketBwServerApp::Run() {
  Init();

  int timeout_ms = prism::GetEnvOrDefault<int>("EPOLL_TIMEOUT_MS", 1000);
  int max_events = prism::GetEnvOrDefault<int>("EPOLL_MAX_EVENTS", 1024);
  // never resize this vector
  std::vector<Event> events(max_events);

  std::queue<std::shared_ptr<BwServerEndpoint>> dead_eps;

  while (1) {
    // Epoll IO
    int nevents = poll_.PollOnce(&events[0], max_events, std::chrono::milliseconds(timeout_ms));

    for (int i = 0; i < nevents; i++) {
      auto& ev = events[i];
      if (ev.token().token == static_cast<size_t>(listener_.sockfd)) {
        RPC_CHECK(ev.IsReadable());
        HandleNewConnection();
        continue;
      }

      // data events
      BwServerEndpoint* endpoint =
          reinterpret_cast<BwServerEndpoint*>(static_cast<uintptr_t>(ev.token()));

      if (ev.IsReadable()) {
        endpoint->OnRecvReady();
      }

      if (ev.IsWritable()) {
        endpoint->OnSendReady();
      }

      if (ev.IsError()) {
        std::string remote_uri = endpoint->sock_addr().AddrStr();
        RPC_LOG(ERROR) << "EPOLLERR, endpoint uri: " << remote_uri;
        endpoint->OnError();
        std::shared_ptr<BwServerEndpoint> resource = endpoints_.extract(endpoint->fd()).mapped();
        dead_eps.push(resource);
      }
    }

    // garbage collection
    while (!dead_eps.empty()) {
      auto resource = std::move(dead_eps.front());
      poll_.registry().Deregister(resource->sock());
      resource->Disconnect();
      dead_eps.pop();
    }
  }

  return 0;
}

}  // namespace socket
}  // namespace rpc_bench
