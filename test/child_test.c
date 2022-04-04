#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ev.h>

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

pid_t child_pid = -1;
pid_t ev_child_pid = -1;

static void
child_cb (EV_P_ ev_child *w, int revents)
{
    ev_child_stop (EV_A_ w);
    printf ("process %d exited with status %x\n", w->rpid, w->rstatus);
}

// exit without attached fd
CTEST(Test, InitTest)
{
    struct ev_loop *loop = EV_DEFAULT;

    ev_child cw;

    pid_t pid = fork();

    if (pid < 0) {
        // error
        perror("fork");
        exit(1);
    }
    else if (pid == 0)
    {
        // the forked child executes here
        exit (0);
    } else
    {
        ev_child_init (&cw, child_cb, pid, 0);
        ev_child_start (loop, &cw);
    }

	ev_run (loop, 0);
}

int main(int argc, const char *argv[])
{
    int result = ctest_main(argc, argv);

    return result;
}
