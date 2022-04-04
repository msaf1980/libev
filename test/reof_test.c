#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ev.h>

#include "utils.h"

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

static ssize_t written = 0, n = -1;
static size_t readed = 0;
static char rbuf[1024];

static struct ev_loop *loop = NULL;

static void ev_sock_close(ev_io *w)
{
    ev_io_stop(loop, w);
    close(w->fd);
}

static void read_cb(EV_P_ ev_io *w, int revents)
{
    n = recv(w->fd, rbuf + readed, sizeof(rbuf) - readed - 1, 0);
    // fprintf(stderr, " (revents: %d, recv: %d)", revents, n);
    if (n < 0)
    {
        perror("recv");
        ev_sock_close(w);
    }
    else if (n == 0)
    {
        ev_sock_close(w);
    }
    else
    {
        readed += (size_t)n;
    }
}

// exit without attached fd
CTEST(Test, REOFTest)
{
    const char *test = "test string";
    ev_io w;

    int pair[2];
    ASSERT_EQUAL(0, socketpair(AF_LOCAL, SOCK_STREAM, 0, pair));

    /* Send some data on socket 0 and immediately close it */

    written = send(pair[0], test, strlen(test), 0);
    ASSERT_TRUE(written > 0);
    shutdown(pair[0], SHUT_WR);

    setnonblock(pair[1]);

    ev_io_init(&w, read_cb, pair[1], EV_READ);
    ev_io_start(loop, &w);

    ev_run(loop, 0);

    // check closed
    ASSERT_EQUAL(0, n);

    // check read results
    ASSERT_EQUAL(written, (ssize_t)readed);
    rbuf[readed] = '\0';
    ASSERT_STR(test, rbuf);
}

int main(int argc, const char *argv[])
{
    loop = EV_DEFAULT;

    int result = ctest_main(argc, argv);

    return result;
}
