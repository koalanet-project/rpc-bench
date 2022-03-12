#ifndef RPC_BENCH_METER_H_
#define RPC_BENCH_METER_H_
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>

#include "logging.h"
#include "prism/utils.h"

namespace rpc_bench {

struct Meter {
  template <typename T>
  using duration = std::chrono::duration<T>;
  using milliseconds = std::chrono::milliseconds;
  using Clock = std::chrono::high_resolution_clock;

  Meter() = default;

  Meter(uint64_t interval_ms, const char* name = "meter", const int _sample = 0xf)
      : name{name}, cnt{0}, bytes{0}, qps{0}, tp{Clock::now()} {
    interval = milliseconds(interval_ms);
    sample = RoundUpPower2(_sample + 1) - 1;
    assert(sample + 1 == Lowbit(sample + 1) && "sample must be 2^n-1");
    disable = prism::GetEnvOrDefault<int>("RPC_BENCH_DISABLE_METER", 0);
  }

  inline void AddBytes(ssize_t add) {
    if (disable) return;
    bytes += add;
    if ((++cnt & sample) == sample) {
      auto now = Clock::now();
      if ((now - tp) >= interval) {
        duration<double> dura = now - tp;
        RPC_LOG(INFO) << prism::FormatString("[%s] Speed: %.6f Mb/s, dura = %.3f ms\n", name,
                                             8.0 * bytes / dura.count() / 1000'000,
                                             dura.count() * 1000);
        fflush(stdout);
        bytes = 0;
        tp = now;
      }
    }
  }

  inline void AddQps(ssize_t bytes_add, ssize_t qps_add) {
    if (disable) return;
    bytes += bytes_add;
    qps += qps_add;
    if ((++cnt & sample) == sample) {
      auto now = Clock::now();
      if ((now - tp) >= interval) {
        duration<double> dura = now - tp;
        printf("[%s] Speed: %.6f Mb/s %.0f ops/s, dura = %.3f ms\n", name,
               8.0 * bytes / dura.count() / 1000'000, qps / dura.count(), dura.count() * 1000);
        fflush(stdout);
        bytes = 0;
        qps = 0;
        tp = now;
      }
    }
  }

  inline long Lowbit(long x) { return x & -x; }

  inline long RoundUpPower2(long x) {
    while (x != Lowbit(x)) x += Lowbit(x);
    return x;
  }

  const char* name;
  int sample;
  int cnt;
  ssize_t bytes;
  ssize_t qps;
  Clock::time_point tp;
  milliseconds interval;
  int disable;
};

}  // namespace rpc_bench

#endif  // RPC_BENCH_METER_H_
