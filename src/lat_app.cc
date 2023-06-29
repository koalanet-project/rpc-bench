#include "lat_app.h"

#include <algorithm>

namespace rpc_bench {

void LatClientApp::print() {
  std::sort(latencies.begin(), latencies.end());
  double sum = 0;
  for (double latency : latencies) sum += latency;
  size_t len = latencies.size();
  printf(
      "%zu\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t "
      "[%zu samples]\n",
      opts_.data_size, sum / len, latencies[(int)(0.5 * len)], latencies[(int)(0.05 * len)],
      latencies[(int)(0.99 * len)], latencies[(int)(0.999 * len)], latencies[(int)(0.9999 * len)],
      latencies[(int)(0.99999 * len)], latencies[(int)(0.999999 * len)], latencies[len - 1], len);
}

}  // namespace rpc_bench
