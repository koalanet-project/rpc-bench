#ifndef RPC_BENCH_CPU_STAT_H_
#define RPC_BENCH_CPU_STAT_H_
#include <stdint.h>
#include <stdio.h>

#include <string>

// https://man7.org/linux/man-pages/man5/proc.5.html

struct CpuSnapshot {
  int pid;
  char comm[16 + 1];
  char state;
  int ppid;
  int pgrp;
  int session;
  int tty_nr;
  int tpgid;
  unsigned flag;
  uint64_t minflt;
  uint64_t cminflt;
  uint64_t majflt;
  uint64_t cmajflt;
  uint64_t utime;
  uint64_t stime;
  int64_t cutime;
  int64_t cstime;

  uint64_t user, nice, sys, idle, iowait, hardirq, softirq, steal, guest, guest_nice;

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

  double CpuUtil() const;

 private:
  CpuSnapshot prev_;
  CpuSnapshot curr_;
};

#endif  // RPC_BENCH_CPU_STAT_H_
