#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include <ev.h>

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

static int notified = 0;
static ev_async notify_watcher;
struct ev_loop *loop = NULL;

static void
notify_cb(EV_P_ ev_async *w, int revents)
{
    notified = 1;
    ev_async_stop(EV_A_ w);
}

static void *notify_thread_proc(void *arg)
{
    /* No other thread is going to join() this one */
    pthread_detach(pthread_self());

    ev_async_send(loop, &notify_watcher);

    pthread_exit(NULL);
}

// exit without attached fd
CTEST(TEST, InitTest)
{
    pthread_t thr;

    ev_async_init(&notify_watcher, notify_cb);
    ev_async_start(loop, &notify_watcher);

    // notify from other thread
    ASSERT_EQUAL(0, pthread_create(&thr, NULL, notify_thread_proc, NULL));

    ev_run(loop, 0);

    ASSERT_EQUAL(1, notified);
}

int main(int argc, const char *argv[])
{
    loop = EV_DEFAULT;

    signal(SIGPIPE, SIG_IGN);

    int result = ctest_main(argc, argv);

    return result;
}
