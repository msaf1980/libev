#ifndef UTILS
#define UTILS

#include <ev.h>

#include <unistd.h>
#include <fcntl.h>

static int setnonblock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

#define ev_timer_restart(loop, w) do { \
        ev_timer_stop(loop, w); \
        ev_timer_start(loop, w); \
    } while (0)

#endif /* UTILS */
