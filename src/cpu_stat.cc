#include "cpu_stat.h"

#include <sstream>

#include "logging.h"

const char* PROC_STAT_PATH = "/proc/stat";

void CpuSnapshot::Snapshot() {
  // open it every time to get new statistics
  FILE* fp = fopen(PROC_STAT_PATH, "r");
  int nreads = fscanf(fp, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &user, &nice, &sys, &idle,
                      &iowait, &hardirq, &softirq, &steal, &guest, &guest_nice);
  RPC_CHECK_EQ(nreads, 10) << "Read CPU statistics failed, nreads: " << nreads;
  fclose(fp);
}


/// please refer to get_per_cpu_interval() in sysstat, rd_stats.c
uint64_t CpuSnapshot::Total() const {
  /*
	 * Don't take cpu_guest and cpu_guest_nice into account
	 * because cpu_user and cpu_nice already include them.
	 */
  return user + nice + sys + idle + iowait + hardirq + softirq + steal;
}

std::string CpuSnapshot::DebugString() {
  std::stringstream ss;
  ss << "{ user: " << user
     << ", nice: " << nice
     << ", sys: " << sys
     << ", idle: " << idle
     << ", iowait: " << iowait
     << ", hardirq: " << hardirq
     << ", softirq: " << softirq
     << ", steal: " << steal
     << ", guest: " << guest
     << ", guest_nice: " << guest_nice << " }";
  return ss.str();
}

CpuStats::CpuStats() {
  Snapshot();
}

void CpuStats::Snapshot() {
  prev_ = curr_;
  curr_.Snapshot();
}

/// please refer to get_per_cpu_interval() in sysstat, rd_stats.c
uint64_t CpuStats::GetCpuInterval(const CpuSnapshot& prev, const CpuSnapshot& curr) {
	uint64_t ishift = 0;

  if ((curr.user - curr.guest) < (prev.user - prev.guest)) {
		/*
		 * Sometimes the nr of jiffies spent in guest mode given by the guest
		 * counter in /proc/stat is slightly higher than that included in
		 * the user counter. Update the interval value accordingly.
		 */
    ishift += (prev.user - prev.guest) - (curr.user - curr.guest);
  }
  if ((curr.nice - curr.guest_nice) < (prev.nice - prev.guest_nice)) {
		/*
		 * Idem for nr of jiffies spent in guest_nice mode.
		 */
    ishift += (prev.nice - prev.guest_nice) - (curr.nice - curr.guest_nice);
  }

  return curr.Total() - prev.Total() + ishift;
}

double CpuStats::AverageIdle() const {
  RPC_CHECK_GT(curr_.idle, 0);
  return (curr_.idle < prev_.idle) ? 0 : 1.0 * (curr_.idle - prev_.idle) / GetCpuInterval();
}

std::string CpuStats::DebugString() {
  std::stringstream ss;
  ss << "{ prev_: " << prev_.DebugString()
     << ", curr_: " << curr_.DebugString() << " }";
  return ss.str();
}
