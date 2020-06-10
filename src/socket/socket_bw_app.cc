#include "socket/socket_bw_app.h"

#include <sys/socket.h>

#include <chrono>
#include <queue>
#include <thread>

#include "bw_app.h"

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

  send_buffer_.resize(kDefaultBufferSize);
  recv_buffer_.resize(kDefaultBufferSize);
}

void SocketBwClientApp::PushData(const BwMessage& bw_msg, BwAck* bw_ack) {
  // serialize and send
  bw_app::PbBwHeader pb_header;
  PackPbBwHeader(bw_msg.header, &pb_header);
  size_t header_size = pb_header.ByteSizeLong();
  pb_header.SerializeToArray(send_buffer_.data(), header_size);
  RPC_CHECK_EQ(header_size, sock_.SendAll(send_buffer_.data(), header_size, 0));
  size_t data_size = bw_msg.data.size();
  RPC_CHECK_EQ(data_size, sock_.SendAll(bw_msg.data.data(), data_size, 0));

  // receive and de-serialize
  ssize_t r = sock_.RecvAll(recv_buffer_.data(), header_size, 0);
  if (r < header_size) {
    RPC_LOG(ERROR) << "only " << r << " vs " << header_size
                   << " bytes received, peer has performed an orderly shutdown";
    bw_ack->success = false;
  } else {
    bw_ack->success = true;
    pb_header.ParseFromArray(recv_buffer_.data(), header_size);
    UnpackPbBwHeader(&bw_ack->header, pb_header);
  }
}

void SocketBwServerApp::Init() {
  AddrInfo ai(opts_.port, SOCK_STREAM);
  listener_.Create(ai);
  listener_.SetReuseAddr(true);
  listener_.SetNonBlock(true);
  listener_.Bind(ai);
  listener_.Listen();

  listen_event_.events = EPOLLIN;
  listen_event_.data.fd = listener_.sockfd;
  poller_.EpollCtl(EPOLL_CTL_ADD, listener_.sockfd, &listen_event_);

  RPC_LOG(DEBUG) << "socket server is listening on uri: " << ai.AddrStr();
}

int SocketBwServerApp::Run() {
  int timeout_ms = prism::GetEnvOrDefault<int>("EPOLL_TIMEOUT_MS", 1000);
  int max_events = prism::GetEnvOrDefault<int>("EPOLL_MAX_EVENTS", 1024);
  std::vector<struct epoll_event> events(max_events);

  std::queue<std::shared_ptr<BwServerEndpoint>> dead_eps;

  while (1) {
    // Epoll IO
    int nevents = poller_.EpollWait(&events[0], max_events, timeout_ms);

    for (int i = 0; i < nevents; i++) {
      auto& ev = events[i];
      if (ev.data.fd == listener_.sockfd) {
        CHECK(ev.events & EPOLLIN);
        HandleNewConnection();
        continue;
      }

      // data events
      BwServerEndpoint* endpoint = static_cast<BwServerEndpoint*>(ev.data.ptr);

      if (ev.events & EPOLLIN) {
        endpoint->OnRecvReady();
      }

      if (ev.events & EPOLLOUT) {
        endpoint->OnSendReady();
      }

      if (ev.events & EPOLLERR) {
        RPC_LOG(ERROR) << "EPOLLERR, endpoint uri: " << endpoint->ai().AddrStr();
        endpoint->OnError();
        dead_eps.push(endpoint);
      }
    }

    // garbage collection
    while (!dead_eps.empty()) {
      auto endpoint = std::move(dead_eps.front());
      /// In kernel versions before 2.6.9, the EPOLL_CTL_DEL operation required
      /// a non-null pointer in event, even though this argument is ignored.
      /// Since Linux 2.6.9, event can be specified as NULL  when  using
      /// EPOLL_CTL_DEL.  Applications that need to be portable to kernels
      /// before 2.6.9 should specify a non-null pointer in event.
      poller_.EpollCtl(EPOLL_CTL_DEL, endpoint->fd(), nullptr);
      dead_eps.pop();
    }
  }

  return 0;
}

}  // namespace socket
}  // namespace rpc_bench
