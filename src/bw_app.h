#ifndef RPC_BENCH_BW_APP_H_
#define RPC_BENCH_BW_APP_H_
#include "app.h"
#include "command_opts.h"
#include "meter.h"
#include "protos/bw_app.pb.h"

namespace rpc_bench {

struct BwHeader {
  uint32_t field1;
  bool field2;
  std::string field3;
  size_t Size() const { return sizeof(field1) + sizeof(field2) + field3.size(); }
};

struct BwMessage {
  BwHeader header;
  std::string data;

  size_t Size() const { return header.Size() + data.size(); }
};

struct BwAck {
  BwHeader header;
  bool success;
};

void PackPbBwMessage(const BwMessage& bw_msg, bw_app::PbBwMessage* pb_bw_msg);

void PackPbBwHeader(const BwHeader bw_header, bw_app::PbBwHeader* pb_bw_header);

void UnpackPbBwHeader(BwHeader* bw_header, const bw_app::PbBwHeader& pb_bw_header);


class BwServerApp : public App {
 public:
  BwServerApp(CommandOpts opts) : App(opts) {}

  virtual int Run() = 0;

  virtual void Init() = 0;

 private:
};

class BwClientApp : public App {
 public:
  BwClientApp(CommandOpts opts) : App(opts), meter_{1000} {}

  virtual int Run() override;

  virtual void Init() = 0;

  /*!
   * \brief push data from client to server
   * \param bw_msg a message that contains the header and additional data (no
   *        need to serialize)
   * \param bw_ack a ack message only contains a copy of the request header
   * 
   * To maximize the bandwidth usage, the API should be designed to return early
   * to overlap the real transmission and serialization.
   */
  virtual void PushData(const BwMessage& bw_msg, BwAck* bw_ack) = 0;

 private:
  Meter meter_;
};

}  // namespace rpc_bench

#endif  // RPC_BENCH_BW_APP_H_
