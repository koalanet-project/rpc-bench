#ifndef RPC_BENCH_SOCKET_EPOLL_HELPER_H_
#define RPC_BENCH_SOCKET_EPOLL_HELPER_H_
#include <sys/epoll.h>

#include "socket/socket.h"

namespace rpc_bench {
namespace socket {

// typedef union epoll_data
// {
//   void *ptr;
//   int fd;
//   uint32_t u32;
//   uint64_t u64;
// } epoll_data_t;

// struct epoll_event
// {
//   uint32_t events;	/* Epoll events */
//   epoll_data_t data;	/* User data variable */
// } __EPOLL_PACKED;

const int INVALID_FD = -1;

class EpollHelper {
 public:
  EpollHelper(int flags) {
    epollfd_ = epoll_create1(flags);
    PCHECK(epollfd_ >= 0) << "epoll_create1 error";
  }

  ~EpollHelper() {
    if (epollfd_ != INVALID_FD) {
      close(epollfd_);
      epollfd_ = INVALID_FD;
    }
  }

  inline void EpollCtl(int op, int fd, struct epoll_event* epev) {
    PCHECK(epoll_ctl(epollfd_, op, fd, epev) == 0);
  }

  inline int EpollWait(struct epoll_event* epev, int maxevents, int timeout_ms) {
    return epoll_wait(epollfd_, epev, maxevents, timeout_ms);
  }

 private:
  int epollfd_{INVALID_FD};
};

}  // namespace socket
}  // namespace rpc_bench

#endif  // RPC_BENCH_SOCKET_EPOLL_HELPER_H_
