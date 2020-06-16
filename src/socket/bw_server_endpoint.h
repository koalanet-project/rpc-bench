#ifndef RPC_BENCH_SOCKET_BW_SERVER_ENDPOINT_H_
#define RPC_BENCH_SOCKET_BW_SERVER_ENDPOINT_H_
#include <queue>
#include <memory>
#include "socket/socket.h"
#include "socket/buffer.h"
#include "socket/poll.h"

namespace rpc_bench {
namespace socket {

class SocketBwServerApp;

/// COMMENT(cjr): before we introduce the concept of `Call`, the TxQueue and
/// RxPoll always have one or two elements at the same time, because there are
/// no concurrent RPC requests, and there is only single type of request.
class BwServerEndpoint {
 public:
  using BufferPtr = std::unique_ptr<Buffer>;
  using TxQueue = std::queue<BufferPtr>;
  using RxPool = std::vector<BufferPtr>;

  // construct from existing socket
  BwServerEndpoint(TcpSocket new_sock, SocketBwServerApp* bw_server_app);

  ~BwServerEndpoint();

  void Disconnect();

  void OnAccepted();

  void OnSendReady();

  void OnRecvReady();

  void OnError();

  void HandleReceivedData(BufferPtr buffer);

  bool ReceiveHeader();
  bool ReceiveData();
  bool Reply();

  inline RawFd fd() const { return sock_; }

  inline const TcpSocket& sock() const { return sock_; }
  inline TcpSocket& sock() { return sock_; }

  inline TxQueue& tx_queue() { return tx_queue_; }

  inline SockAddr& sock_addr() { return sock_addr_; }

 private:
  void GetPeerAddr();

  // socket handle
  TcpSocket sock_;
  // Connection Metadata
  SocketBwServerApp* app_;
  SockAddr sock_addr_;
  // datapath I/O
  Interest interest_;
  TxQueue tx_queue_;
  RxPool rx_pool_;
  BufferPtr header_buffer_;
  BufferPtr data_buffer_;
  BufferPtr reply_buffer_;
  // receiving state, this should be replaced with something like a generator or
  // a coroutine
  enum class State {
    NEW_RPC,
    HEADER_RECVED,
    DATA_RECVED,
  };

  State state_;
};

}  // namespace socket
}  // namespace rpc_bench

#endif // RPC_BENCH_SOCKET_BW_SERVER_ENDPOINT_H_
