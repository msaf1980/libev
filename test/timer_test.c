#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <ev.h>

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

static int timed;

// another callback, this time for a time-out
static void
timeout_cb (EV_P_ ev_timer *w, int revents)
{
    if (timed++ > 0)
    {
        // this causes the innermost ev_run to stop iterating
        ev_break (EV_A_ EVBREAK_ONE);
    }
}

CTEST(Test, TimerTest)
{
    ev_timer timeout1_watcher;
    ev_timer timeout2_watcher;

    struct ev_loop *loop = EV_DEFAULT;

	ev_timer_init(&timeout1_watcher, timeout_cb, 0.01, 0.);	
    ev_timer_start(loop, &timeout1_watcher);
    ev_timer_init(&timeout2_watcher, timeout_cb, 0.1, 0.);	
    ev_timer_start(loop, &timeout2_watcher);

    timed = 0;

	ev_run (loop, 0);

    ASSERT_EQUAL(2, timed);
}

int main(int argc, const char *argv[])
{
    int result = ctest_main(argc, argv);

    return result;
}
