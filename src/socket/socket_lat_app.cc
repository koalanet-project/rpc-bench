#include "socket_lat_app.h"

#include <limits>
#include <thread>

#include "logging.h"
#include "prism/utils.h"
#include "socket/poll.h"

namespace rpc_bench {
namespace socket {

void SocketLatClientApp::Init() {
  AddrInfo ai(opts_.host.value().c_str(), opts_.port, SOCK_STREAM);
  RPC_LOG(DEBUG) << "socket client is connecting to remote_uri: " << ai.AddrStr();
  sock_.Create(ai);
  sock_.Connect(ai);
  sock_.SetNonBlock(true);
  RPC_LOG(DEBUG) << "connected to: " << ai.AddrStr();
}

int SocketLatClientApp::Run() { return 0; }

void SocketLatServerApp::Init() {
  AddrInfo ai(opts_.port, SOCK_STREAM);
  listener_.Create(ai);
  listener_.SetReuseAddr(true);
  listener_.SetNonBlock(true);
  listener_.Bind(ai);
  listener_.Listen();

  RPC_LOG(DEBUG) << "socket server is listening on uri: " << ai.AddrStr();
}


int SocketLatServerApp::Run() {
  Init();
}

}  // namespace socket
}  // namespace rpc_bench
