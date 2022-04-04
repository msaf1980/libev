#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <ev.h>

static ssize_t count, fired;
static int writes, failures;
static int *pipes;
static int num_pipes, num_active, num_writes;
static ev_io *events;
static ev_timer *timers;

struct ev_loop *loop;

// #define	timersub(tvp, uvp, vvp)						\
// 	do {								\
// 		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
// 		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
// 		if ((vvp)->tv_usec < 0) {				\
// 			(vvp)->tv_sec--;				\
// 			(vvp)->tv_usec += 1000000;			\
// 		}							\
// 	} while (0)


static void
timer_cb(EV_P_ ev_timer *w, int revents) {}

static void
read_cb(EV_P_ ev_io *w, int revents)
{
	int idx = (int) w->data, widx = idx + 1;
	unsigned char ch;
	ssize_t n;

	n = recv(w->fd, (char*)&ch, sizeof(ch), 0);
    // fprintf(stderr, "%d %d: recv %lld (%lld)\n", (long long) w, w->fd, w->data, n);
	if (n >= 0)
		count += n;
	else
		failures++;
	if (writes) {
		if (widx >= num_pipes)
			widx -= num_pipes;
		n = send(pipes[2 * widx + 1], "e", 1, 0);
		if (n != 1)
			failures++;
		writes--;
		fired++;
	}
}

static struct timeval *
run_once(void)
{
	int *cp, space;
	long i;
	static struct timeval ts, te;

	for (cp = pipes, i = 0; i < num_pipes; i++, cp += 2) {
		if (ev_is_active(&events[i])) {
            ev_io_stop (loop, &events[i]);
        }
        ev_io_init(&events[i], read_cb, cp[0], EV_READ);
        ev_io_start(loop, &events[i]);

        timers[i].repeat = 10. + drand48 ();
        ev_timer_again(loop, &timers[i]);

        // fprintf(stderr, "%d %d: event add %lld\n", &events[i], events[i].fd, events[i].data);
	}

	// ev_loop(loop, EVRUN_ONCE | EVRUN_NOWAIT);

	fired = 0;
	space = num_pipes / num_active;
	space = space * 2;
	for (i = 0; i < num_active; i++, fired++) {
		ssize_t n = send(pipes[i * space + 1], "e", 1, 0);
        // fprintf(stderr, "%d: send %lld\n", pipes[i * space + 1], (long long) n);        
    }

	count = 0;
	writes = num_writes;
	{
		int xcount = 0;
		gettimeofday(&ts, NULL);
		do {
			// fprintf(stderr, "loop Xcount: %lld, Rcount: %lld\n",
			// 	 (long long)xcount, (long long) count);
			ev_loop(loop, EVRUN_ONCE);
			xcount++;
		} while (count != fired);
		gettimeofday(&te, NULL);

		if (xcount != count)
			fprintf(stderr, "Xcount: %lld, Rcount: %lld\n",
				 (long long)xcount, (long long) count);
	}

	timersub(&te, &ts, &te);

	return (&te);
}

int
main(int argc, char **argv)
{
#ifdef EVENT__HAVE_SETRLIMIT
	struct rlimit rl;
#endif
	int i, c;
	struct timeval *tv;
	int *cp;
	// const char **methods;
	// const char *method = NULL;
	// struct event_config *cfg = NULL;

	num_pipes = 100;
	num_active = 1;
	num_writes = num_pipes;
	// while ((c = getopt(argc, argv, "n:a:w:m:l")) != -1) {
    while ((c = getopt(argc, argv, "n:a:w:")) != -1) {
		switch (c) {
		case 'n':
			num_pipes = atoi(optarg);
			break;
		case 'a':
			num_active = atoi(optarg);
			break;
		case 'w':
			num_writes = atoi(optarg);
			break;
		// case 'm':
		// 	method = optarg;
		// 	break;
		// case 'l':
		// 	methods = event_get_supported_methods();
		// 	fprintf(stdout, "Using Libevent %s. Available methods are:\n",
		// 		event_get_version());
		// 	for (i = 0; methods[i] != NULL; ++i)
		// 		printf("    %s\n", methods[i]);
		// 	exit(0);
		default:
			fprintf(stderr, "Illegal argument \"%c\"\n", c);
			exit(1);
		}
	}

#ifdef EVENT__HAVE_SETRLIMIT
	rl.rlim_cur = rl.rlim_max = num_pipes * 2 + 50;
	if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
		perror("setrlimit");
		exit(1);
	}
#endif

	events = calloc(num_pipes, sizeof(struct ev_io));
    timers = calloc(num_pipes, sizeof(struct ev_timer));
	pipes = calloc(num_pipes * 2, sizeof(int));
	if (events == NULL || timers == NULL || pipes == NULL) {
		perror("alloc");
		exit(1);
	}

	// if (method != NULL) {
	// 	cfg = event_config_new();
	// 	methods = event_get_supported_methods();
	// 	for (i = 0; methods[i] != NULL; ++i)
	// 		if (strcmp(methods[i], method))
	// 			event_config_avoid_method(cfg, methods[i]);
	// 	base = event_base_new_with_config(cfg);
	// 	event_config_free(cfg);
	// } else
	// 	base = event_base_new();

	for (cp = pipes, i = 0; i < num_pipes; i++, cp += 2) {
#ifdef USE_PIPES
		if (pipe(cp) == -1) {
#else
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, cp) == -1) {
#endif
			perror("pipe");
			exit(1);
		}

        ev_io_init(&events[i], read_cb, cp[0], EV_READ);
        events[i].data = (void *)i;

        ev_timer_init(&timers[i], timer_cb, 1.0, 0);        
	}

    loop = EV_DEFAULT;

	for (i = 0; i < 25; i++) {
		tv = run_once();
		if (tv == NULL)
			exit(1);
		fprintf(stdout, "%ld ms (total %d, active %d)\n",
			tv->tv_sec * 1000000L + tv->tv_usec, num_pipes, num_active);
	}

	exit(0);
}
