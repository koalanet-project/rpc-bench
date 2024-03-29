#ifndef RPC_BENCH_SOCKET_BW_APP_H_
#define RPC_BENCH_SOCKET_BW_APP_H_
#include <vector>
#include <map>

#include "bw_app.h"
#include "command_opts.h"
#include "socket/bw_server_endpoint.h"
#include "socket/socket.h"
#include "socket/poll.h"

namespace rpc_bench {
namespace socket {

class SocketBwClientApp final : public BwClientApp {
 public:
  const size_t kDefaultBufferSize = 4194304;
  SocketBwClientApp(CommandOpts opts) : BwClientApp(opts), sock_{INVALID_SOCKET} {}
  ~SocketBwClientApp() {
    sock_.Close();
  }

  virtual void Init() override;

  virtual void PushData(const BwMessage& bw_msg, BwAck* bw_ack) override;

 private:
  TcpSocket sock_;
  std::vector<uint8_t> send_buffer_;
  std::vector<uint8_t> recv_buffer_;
};

class BwServerEndpoint;

class SocketBwServerApp final : public BwServerApp {
 public:
  SocketBwServerApp(CommandOpts opts)
      : BwServerApp(opts), listener_{INVALID_SOCKET}, poll_{Poll::Create()} {}

  virtual void Init() override;

  virtual int Run() override;

  inline Poll& poll() { return poll_; }

 private:
  void HandleNewConnection();

  TcpSocket listener_;
  Poll poll_;
  std::map<RawFd, std::shared_ptr<BwServerEndpoint>> endpoints_;
};

}  // namespace socket
}  // namespace rpc_bench

#endif  // RPC_BENCH_SOCKET_BW_APP_H_
