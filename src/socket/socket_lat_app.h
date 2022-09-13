#ifndef RPC_BENCH_SOCKET_LAT_APP_H_
#define RPC_BENCH_SOCKET_LAT_APP_H_

#include "command_opts.h"
#include "lat_app.h"
#include "protos/lat_tput_app.grpc.pb.h"
#include "protos/lat_tput_app.pb.h"
#include "socket.h"

namespace rpc_bench {
namespace socket {

using ::rpc_bench::lat_tput_app::Ack;
using ::rpc_bench::lat_tput_app::Data;

class SocketLatClientApp final : public LatClientApp {
 public:
  SocketLatClientApp(CommandOpts opts) : LatClientApp(opts) {}

  virtual void Init() override;

  virtual int Run() override;

 private:
  TcpSocket sock_;
};

class SocketLatServerApp final : public LatServerApp {
 public:
  SocketLatServerApp(CommandOpts opts) : LatServerApp(opts) {}

  virtual void Init() override;

  virtual int Run() override;

 private:
  void HandleNewConnection();

  TcpSocket listener_;
};

}  // namespace socket
}  // namespace rpc_bench

#endif