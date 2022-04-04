#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <ev.h>

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

// exit without attached fd
CTEST(Test, InitTest)
{
    struct ev_loop *loop = EV_DEFAULT;

	ev_run(loop, 0);
}

int main(int argc, const char *argv[])
{
    int result = ctest_main(argc, argv);

    return result;
}
