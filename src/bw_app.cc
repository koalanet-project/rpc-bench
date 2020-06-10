#include "bw_app.h"

namespace rpc_bench {

using bw_app::PbBwAck;
using bw_app::PbBwHeader;
using bw_app::PbBwMessage;

int BwClientApp::Run() {
  Init();

  /// it should be sufficient to saturate the bandwidth using synchronous mode
  /// with large message per request. If that is not the case, there must be
  /// somewhere inefficient implementation (e.g. unnecessarry copy, context
  /// switch of task spreading, or fail to pipelining serialization and network IO).
  BwMessage bw_msg;
  bw_msg.data.resize(opts_.data_size);
  auto start = std::chrono::high_resolution_clock::now();
  auto time_dura = std::chrono::microseconds(static_cast<long>(opts_.time_duration_sec * 1e6));
  while (1) {
    BwAck ack;
    PushData(bw_msg, &ack);
    meter_.AddBytes(bw_msg.Size());
    auto now = std::chrono::high_resolution_clock::now();
    if (now - start > time_dura) {
      break;
    }
  }
  return 0;
}

void PackPbBwMessage(const BwMessage& bw_msg, PbBwMessage* pb_bw_msg) {
  // TODO(cjr): this may have some problems
  const BwHeader& header = bw_msg.header;
  PackPbBwHeader(header, pb_bw_msg->mutable_header());
  pb_bw_msg->set_data(bw_msg.data);
}

void UnpackPbBwHeader(BwHeader* bw_header, const PbBwHeader& pb_bw_header) {
  bw_header->field1 = pb_bw_header.field1();
  bw_header->field2 = pb_bw_header.field2();
  bw_header->field3 = pb_bw_header.field3();
}

void PackPbBwHeader(const BwHeader bw_header, PbBwHeader* pb_bw_header) {
  pb_bw_header->set_field1(bw_header.field1);
  pb_bw_header->set_field2(bw_header.field2);
  pb_bw_header->set_field3(bw_header.field3);
}

}  // namespace rpc_bench
