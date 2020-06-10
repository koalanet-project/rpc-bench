#ifndef RPC_BENCH_SOCKET_BW_SERVER_ENDPOINT_H_
#define RPC_BENCH_SOCKET_BW_SERVER_ENDPOINT_H_
#include "socket/socket.h"
#include "socket/epoll_helper.h"

namespace rpc_bench {
namespace socket {

class BwServerEndpoint {
 public:
  using TxQueue = std::queue<std::unique_ptr<Buffer>>;

  BwServerEndpoint();

  // construct from existing socket
  BwServerEndpoint(TcpSocket new_sock);

  ~BwServerEndpoint();

  void Connect();

  void Disconnect();

  void OnConnected();

  void OnAccepted();

  void OnSendReady();

  void OnRecvReady();

  void OnError();

  void HandleReceivedData(std::unique_ptr<Buffer> buffer);

  inline int fd() const { return sock_; }

  inline const TcpSocket& sock() const { return sock_; }
  inline TcpSocket& sock() { return sock_; }

  inline const struct epoll_event& event() const { return event_; }
  inline struct epoll_event& event() { return event_; }

  inline void set_dead(bool is_dead) { is_dead_ = is_dead; }
  inline bool is_dead() const { return is_dead_; }

  inline TxQueue& tx_queue() { return tx_queue_; }

 private:
  TcpSocket sock_;
  struct epoll_event event_;
  bool is_dead_ {false};
  TxQueue tx_queue_;
  std::vector<std::unique_ptr<Buffer>> rx_buffers_;
};

}  // namespace socket
}  // namespace rpc_bench

#endif // RPC_BENCH_SOCKET_BW_SERVER_ENDPOINT_H_
