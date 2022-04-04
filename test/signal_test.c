#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ev.h>

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

static int signalled = 0;

static void
sigint_cb (struct ev_loop *loop, ev_signal *w, int revents)
{
    signalled = 1;
    ev_break (loop, EVBREAK_ALL);
}

static void
timer_cb (EV_P_ ev_timer *w, int revents)
{
    pid_t pid = getpid();
    kill(pid, SIGINT);
}

CTEST(Test, SignalTest)
{
    struct ev_loop *loop = EV_DEFAULT;

    ev_signal signal_watcher;
    ev_timer timer_watcher;

    ev_signal_init(&signal_watcher, sigint_cb, SIGINT);
    ev_signal_start(loop, &signal_watcher);

    ev_timer_init(&timer_watcher, timer_cb, 0.01, 0.);	
    ev_timer_start(loop, &timer_watcher);

	ev_run (loop, 0);

    ASSERT_EQUAL(1, signalled);
}

int main(int argc, const char *argv[])
{
    int result = ctest_main(argc, argv);

    return result;
}
