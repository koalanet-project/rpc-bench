#include "bw_app.h"

namespace rpc_bench {

int BwClientApp::Run() {
  Init();

  BwMessage bw_msg;
  bw_msg.data.resize(opts_.data_size);
  while (1) {
    BwAck ack;
    IssueBwReq(bw_msg, &ack);
    meter_.AddBytes(bw_msg.size());
  }
  return 0;
}

}  // namespace rpc_bench
