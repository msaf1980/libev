#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ev.h>

#include "utils.h"

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

static ssize_t written = 0, readed = 0, n = -1, writes = 0;
static char wbuf[1024];
static size_t wsize;

struct ev_loop *loop = NULL;

static void ev_sock_close(ev_io *w) {
    ev_io_stop(loop, w);
    close(w->fd);
}

static void read_cb(EV_P_ ev_io *w, int revents) {
    // immedially close read socket for simulate write EOF
    ev_sock_close(w);
}

static void write_cb(EV_P_ ev_io *w, int wevents) {
    n = send(w->fd, wbuf+written, wsize-written, 0);
    // fprintf(stderr, " (wevents: %d, send: %d)", wevents, n);    
    if (n < 0) {
        //fprintf(stderr, "send: %s", strerror(errno));
        //Broken pipe
        ev_sock_close(w);
    } else if (n == 0) {
        ev_sock_close(w);
    } else {
        written += (size_t) n;
        writes++;
    }
}

// exit without attached fd
CTEST(Test, WEOFTest) {
  ev_io w_read;
  ev_io w_write;

  int pair[2];
  ASSERT_EQUAL(0, socketpair(AF_LOCAL, SOCK_STREAM, 0, pair));

  strcpy(wbuf, "test string");
  wsize = strlen(wbuf);

  setnonblock(pair[0]);
  setnonblock(pair[1]);

  ev_io_init(&w_read, read_cb, pair[1], EV_READ);
  ev_io_start(loop, &w_read);

  ev_io_init(&w_write, write_cb, pair[0], EV_WRITE);
  ev_io_start(loop, &w_write);

  ev_run(loop, 0);

  // check write error
  ASSERT_EQUAL(-1, n);

  // check write results - first write success, we can't detect closed peer at this step
  ASSERT_EQUAL(1, writes);
}

int main(int argc, const char *argv[])
{
    loop = EV_DEFAULT;

    signal(SIGPIPE, SIG_IGN);

    int result = ctest_main(argc, argv);

    return result;
}
