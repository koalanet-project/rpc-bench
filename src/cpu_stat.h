#ifndef RPC_BENCH_CPU_STAT_H_
#define RPC_BENCH_CPU_STAT_H_
#include <stdint.h>
#include <stdio.h>

#include <string>

/// man proc(5)
//        /proc/stat
//               kernel/system statistics.  Varies with architecture.  Common entries include:
//
//               cpu 10132153 290696 3084719 46828483 16683 0 25195 0 175628 0
//               cpu0 1393280 32966 572056 13343292 6130 0 17875 0 23933 0
//                      The amount of time, measured in units of USER_HZ (1/100ths of a second on
//                      most architectures, use sysconf(_SC_CLK_TCK) to obtain the right value),
//                      that the system ("cpu" line) or the specific CPU ("cpuN" line) spent in
//                      various states:
//
//                      user   (1) Time spent in user mode.
//
//                      nice   (2) Time spent in user mode with low priority (nice).
//
//                      system (3) Time spent in system mode.
//
//                      idle   (4) Time spent in the idle task.  This value should be USER_HZ times
//                      the second entry in the /proc/uptime pseudo-file.
//
//                      iowait (since Linux 2.5.41)
//                             (5) Time waiting for I/O to complete.  This value is not reliable,
//                             for the following reasons:
//
//                             1. The CPU will not wait for I/O to complete; iowait is the time that
//                             a task is waiting for I/O to complete.  When a CPU goes into idle
//                             state for outstanding task I/O, another task will be scheduled on
//                             this CPU.
//
//                             2. On a multi-core CPU, the task waiting for I/O to complete is not
//                             running on any CPU, so the iowait of each CPU is difficult to
//                             calculate.
//
//                             3. The value in this field may decrease in certain conditions.
//
//                      irq (since Linux 2.6.0)
//                             (6) Time servicing interrupts.
//
//                      softirq (since Linux 2.6.0)
//                             (7) Time servicing softirqs.
//
//                      steal (since Linux 2.6.11)
//                             (8) Stolen time, which is the time spent in other operating systems
//                             when running in a virtualized environment
//
//                      guest (since Linux 2.6.24)
//                             (9) Time spent running a virtual CPU for guest operating systems
//                             under the control of the Linux kernel.
//
//                      guest_nice (since Linux 2.6.33)
//                             (10) Time spent running a niced guest (virtual CPU for guest
//                             operating systems under the control of the Linux kernel).
//
struct CpuSnapshot {
  uint64_t user;
  uint64_t nice;
  uint64_t sys;
  uint64_t idle;
  uint64_t iowait;
  uint64_t hardirq;
  uint64_t softirq;
  uint64_t steal;
  uint64_t guest;
  uint64_t guest_nice;

  void Snapshot();
  std::string DebugString();

  uint64_t Total() const;
};

class CpuStats {
 public:
  CpuStats();

  void Snapshot();

  static uint64_t GetCpuInterval(const CpuSnapshot& prev, const CpuSnapshot& curr);

  uint64_t GetCpuInterval() const { return GetCpuInterval(prev_, curr_); }

  double AverageIdle() const;

  std::string DebugString();

 private:
  CpuSnapshot prev_;
  CpuSnapshot curr_;
};

#endif  // RPC_BENCH_CPU_STAT_H_
