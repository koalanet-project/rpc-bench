#ifndef RPC_BENCH_LOG_H_
#define RPC_BENCH_LOG_H_

#include <glog/logging.h>

#define DEBUG INFO
#define TRACE INFO

#define RPC_LOG(severity) LOG(severity)

#define RPC_LOG_IF(severity, condition) LOG_IF(severity, (condition))

#define RPC_CHECK(x) CHECK(x)
#define RPC_CHECK_LT(x, y) CHECK_LT(x, y)
#define RPC_CHECK_GT(x, y) CHECK_GT(x, y)
#define RPC_CHECK_LE(x, y) CHECK_LE(x, y)
#define RPC_CHECK_GE(x, y) CHECK_GE(x, y)
#define RPC_CHECK_EQ(x, y) CHECK_EQ(x, y)
#define RPC_CHECK_NE(x, y) CHECK_NE(x, y)
#define RPC_CHECK_NOTNULL(x) CHECK_NOTNULL(x)

#define RPC_UNIMPLEMENTED RPC_LOG(FATAL) << "unimplemented\n";

#endif
