#include "cpu_stat.h"

#include <sstream>
#include <thread>

#include "logging.h"

const char* PROC_STAT_PATH = "/proc/stat";
const char* PROC_SELF_STAT_PATH = "/proc/self/stat";

uint64_t CpuSnapshot::Total() const {
  /*
   * Don't take cpu_guest and cpu_guest_nice into account
   * because cpu_user and cpu_nice already include them.
   */
  return user + nice + sys + idle + iowait + hardirq + softirq + steal;
}

// https://stackoverflow.com/questions/16726779/how-do-i-get-the-total-cpu-usage-of-an-application-from-proc-pid-stat
void CpuSnapshot::Snapshot() {
  // open it every time to get new statistics

  FILE* fp = fopen(PROC_STAT_PATH, "r");
  int nread = fscanf(fp, " cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &user, &nice, &sys, &idle,
                     &iowait, &hardirq, &softirq, &steal, &guest, &guest_nice);
  CHECK(nread == 10);
  fclose(fp);

  fp = fopen(PROC_SELF_STAT_PATH, "r");
  nread = fscanf(fp, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld", &pid, comm,
                 &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flag, &minflt, &cminflt, &majflt,
                 &cmajflt, &utime, &stime, &cutime, &cstime);
  CHECK(nread == 17);
  fclose(fp);
}

std::string CpuSnapshot::DebugString() {
  std::stringstream ss;
  char s[512];
  sprintf(s,
          "{ pid: %d, comm: %s, state: %c, ppid: %d, pgrp: %d, session: %d, tty_nr: %d, tpgid: "
          "%d, flag: %u, minflt: %lu, cminflt: %lu, majflt: %lu, cmajflt: %lu, utime: %lu, stime: "
          "%lu, cutime: %ld, cstime: %ld }\n",
          pid, comm, state, ppid, pgrp, session, tty_nr, tpgid, flag, minflt, cminflt, majflt,
          cmajflt, utime, stime, cutime, cstime);
  ss << s;
  return ss.str();
}

CpuStats::CpuStats() { Snapshot(); }

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
  ss << "{ prev_: " << prev_.DebugString() << ", curr_: " << curr_.DebugString() << " }";
  return ss.str();
}

double CpuStats::CpuUtil() const {
  uint64_t u = curr_.utime - prev_.utime;
  uint64_t s = curr_.stime - prev_.stime;
  uint64_t t = curr_.Total() - prev_.Total();

  const unsigned core_cnt = std::thread::hardware_concurrency();

  return 1.0 * (u + s) / t * core_cnt;
}
