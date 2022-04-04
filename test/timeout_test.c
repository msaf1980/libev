#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <ev.h>

#include "utils.h"

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"
#include "ctest_fmt.h"

static ssize_t written = 0;
static size_t readed = 0, writes = 0;
static char rbuf[1024];
static double timeout = 0.1;

static struct ev_loop *loop = NULL;

#define EV_SOCKET_TIMEOUT -2

typedef struct ev_socket_
{
    ev_io w;
    ev_timer t;
    ssize_t last;
} ev_socket;

static void ev_socket_close(ev_socket *s)
{
    ev_io_stop(loop, &s->w);
    ev_timer_stop(loop, &s->t);
    close(s->w.fd);
}

static void
timeout_cb(EV_P_ ev_timer *w, int revents)
{
    ev_socket *client = (ev_socket *)(((char *)w) - offsetof(ev_socket, t));
    // fprintf(stderr, " (fd: %d, revents: %d, timeout %.2f)", client->w.fd, revents, timeout);
    ev_socket_close(client);
    client->last = EV_SOCKET_TIMEOUT;
}

static void read_cb(EV_P_ ev_io *w, int revents)
{
    ev_socket *client = (ev_socket *)w;
    client->last = recv(client->w.fd, rbuf + readed, sizeof(rbuf) - readed - 1, 0);
    // fprintf(stderr, " (fd: %d, revents: %d, recv: %lld)", w->fd, revents, (long long) client->last);
    if (client->last < 0)
    {
        if (errno != EINTR && errno != EAGAIN)
        {
            perror("recv");
            ev_socket_close(client);
        }
    }
    else if (client->last == 0)
    {
        ev_socket_close(client);
    }
    else
    {
        readed += (size_t)client->last;
        ev_timer_restart(loop, &client->t);
    }
}

static void write_cb(EV_P_ ev_io *w, int wevents)
{
    ev_socket *client = (ev_socket *)w;
    client->last = send(w->fd, "test", 4, 0);
    // fprintf(stderr, " (wevents: %d, send: %d)", wevents, client->last);
    if (client->last < 0)
    {
        if (errno != EINTR && errno != EAGAIN)
        {
            // fprintf(stderr, "send: %s", strerror(errno));
            // Broken pipe
            ev_socket_close(client);
        }
    }
    else if (client->last == 0)
    {
        ev_socket_close(client);
    }
    else
    {
        written += (size_t)client->last;
        writes++;
        ev_timer_restart(loop, &client->t);
    }
}

CTEST(Test, ReadTimeoutTest)
{
    char buf[128];

    const char *test = "test string";
    ev_socket client;
    struct timespec start, end;
    double duration;

    int pair[2];
    ASSERT_EQUAL(0, socketpair(AF_LOCAL, SOCK_STREAM, 0, pair));

    written = 0;
    readed = 0;

    setnonblock(pair[0]);
    setnonblock(pair[1]);

    /* Send some data on socket 0 */

    written = send(pair[0], test, strlen(test), 0);
    ASSERT_TRUE(written > 0);

    client.last = 0;
    ev_io_init(&client.w, read_cb, pair[1], EV_READ);
    ev_io_start(loop, &client.w);

    ev_timer_init(&client.t, timeout_cb, timeout, 0.);
    ev_timer_start(loop, &client.t);

    clock_gettime(CLOCK_REALTIME, &start);
    ev_run(loop, 0);
    clock_gettime(CLOCK_REALTIME, &end);

    // check read results
    ASSERT_EQUAL(written, (ssize_t)readed);
    rbuf[readed] = '\0';
    ASSERT_STR(test, rbuf);

    // check last error and timeout
    ASSERT_EQUAL(EV_SOCKET_TIMEOUT, client.last);
    duration = (double)(end.tv_sec * 1000000000.0 - start.tv_sec * 1000000000.0 + end.tv_nsec - start.tv_nsec) / 1000000000.0;
    ASSERT_DBL_NEAR_TOL_D(timeout, duration, 0.1, CTEST_FORMAT(buf, sizeof(buf), "want timeout %l < %l", duration, timeout));
}

CTEST(Test, WriteTimeoutTest)
{
    char buf[128];
    
    const char *test = "test string";
    ev_socket client;
    struct timespec start, end;
    double duration;

    int pair[2];
    ASSERT_EQUAL(0, socketpair(AF_LOCAL, SOCK_STREAM, 0, pair));

    written = 0;
    writes = 0;
    readed = 0;

    setnonblock(pair[0]);
    setnonblock(pair[1]);

    client.last = 0;
    ev_io_init(&client.w, write_cb, pair[0], EV_WRITE);
    ev_io_start(loop, &client.w);

    ev_timer_init(&client.t, timeout_cb, timeout, 0.);
    ev_timer_start(loop, &client.t);

    clock_gettime(CLOCK_REALTIME, &start);
    ev_run(loop, 0);
    clock_gettime(CLOCK_REALTIME, &end);

    close(pair[0]);

    // check write error - some writes success, but when sent buffers overlowed - timeouted - remote peer not read from socket
    // check last error and timeout
    ASSERT_EQUAL(EV_SOCKET_TIMEOUT, client.last);
    duration = (double)(end.tv_sec * 1000000000.0 - start.tv_sec * 1000000000.0 + end.tv_nsec - start.tv_nsec) / 1000000000.0;
    ASSERT_DBL_NEAR_TOL_D(timeout, duration, 0.1, CTEST_FORMAT(buf, sizeof(buf), "want timeout %l < %l", duration, timeout));
}

int main(int argc, const char *argv[])
{
    loop = EV_DEFAULT;

    int result = ctest_main(argc, argv);

    return result;
}
