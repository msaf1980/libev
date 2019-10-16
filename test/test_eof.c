#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ev.h>

#define GO_LABEL(code, label)                                                  \
	do {                                                                       \
		rc = (code);                                                           \
		goto label;                                                            \
	} while (0);

int rc = 0;

int set_nonblock(int fd) {
	int flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// another callback, this time for a time-out
static void timeout_cb(EV_P_ ev_timer *w, int revents) {
	puts("timeout");
	// this causes the innermost ev_run to stop iterating
	ev_break(EV_A_ EVBREAK_ONE);
}

static void read_cb(EV_P_ ev_io *w, int revents) {
	char   b[256];
	size_t n;
	ev_io_stop(EV_A_ w);
	memset(b, 0, sizeof(b));
	if ((n = read(w->fd, b, sizeof(b) - 1)) > 0) {
		printf("read: %s\n", b);
		rc = 0;
		ev_io_start(EV_A_ w);
	} else {
		close(w->fd);
		printf("close: %zd\n", n);
		rc = n;
		ev_break(EV_A_ EVBREAK_ONE);
	}
}

static void write_cb(EV_P_ ev_io *w, int revents) {
	char *b = "a";
	printf("write: %s\n", b);
	ev_io_stop(EV_A_ w);
	write(w->fd, b, 1);
	close(w->fd);
}

int main(int argc, char *argv[]) {
	int pfd[2];

	ev_io    read_watcher;
	ev_io    write_watcher;
	ev_timer timeout_watcher;

	if (pipe(pfd) == -1) {
		perror("pipe");
		GO_LABEL(-1, EXIT);
	}

	set_nonblock(pfd[0]);
	set_nonblock(pfd[1]);

	rc = 1;

	struct ev_loop *loop = EV_DEFAULT;

	/* start read watcher */
	ev_io_init(&read_watcher, read_cb, pfd[0], EV_READ);
	ev_io_start(loop, &read_watcher);

	/* start write watcher */
	ev_io_init(&write_watcher, write_cb, pfd[1], EV_WRITE);
	ev_io_start(loop, &write_watcher);

	/* initialise a timer watcher, then start non-repeating 5.0 second timeout
	 */
	ev_timer_init(&timeout_watcher, timeout_cb, 5.0, 0.);
	ev_timer_start(loop, &timeout_watcher);

	ev_run(loop, 0);
EXIT:
	return rc;
}
