/* read/write and timeout */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
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

typedef struct {
	ev_io    watcher;
	ev_timer timer;
	char     buf[256];
} io_t;

int ignore_sigpipe() {
	struct sigaction sa, osa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	return sigaction(SIGPIPE, &sa, &osa);
}

int set_nonblock(int fd) {
	int flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* another callback, this time for a time-out */
static void timeout_cb(EV_P_ ev_timer *w, int revents) {
	io_t *client = (io_t *) (((char *) w) - offsetof(io_t, timer));
	ev_timer_stop(EV_A_ w);
	puts("timeout");
	if (rc == 1)
		rc = 0;
	/* close socket */
	close(client->watcher.fd);
}

static void read_cb(EV_P_ ev_io *w, int revents) {
	io_t * client = (io_t *) (((char *) w) - offsetof(io_t, watcher));
	ssize_t n;
	ev_io_stop(EV_A_ w);
	if ((n = read(w->fd, client->buf, sizeof(client->buf))) > 0) {
		if (rc == -1)
			rc = 1;
		ev_io_start(EV_A_ w);
	} else {
		close(w->fd);
		printf("close reader: %zd, revents: %d\n", n, revents);
		ev_break(EV_A_ EVBREAK_ALL);
	}
}

static void write_cb(EV_P_ ev_io *w, int revents) {
	io_t * client = (io_t *) (((char *) w) - offsetof(io_t, watcher));
	ssize_t n;
	ev_io_stop(EV_A_ w);
	if ((n = write(w->fd, client->buf, sizeof(client->buf))) > 0) {
		ev_io_start(EV_A_ w);
	} else {
		close(w->fd);
		printf("close writer: %zd, revents: %d\n", n, revents);
	}
}

int main(int argc, char *argv[]) {
	int pfd[2];

	io_t     reader;
	io_t     writer;

	ignore_sigpipe();

	if (pipe(pfd) == -1) {
		perror("pipe");
		GO_LABEL(-1, EXIT);
	}

	set_nonblock(pfd[0]);
	set_nonblock(pfd[1]);

	rc = -1;

	struct ev_loop *loop = EV_DEFAULT;

	/* start read watcher */
	ev_io_init(&reader.watcher, read_cb, pfd[0], EV_READ);
	ev_io_start(loop, &reader.watcher);

	/* start write watcher */
	ev_io_init(&writer.watcher, write_cb, pfd[1], EV_WRITE);
	ev_io_start(loop, &writer.watcher);

	/* initialise a timer watcher, then start non-repeating 5.0 second timeout
	 */
	ev_timer_init(&reader.timer, timeout_cb, .1, 0.);
	ev_timer_start(loop, &reader.timer);

	ev_timer_init(&writer.timer, timeout_cb, .2, 0.);
	ev_timer_start(loop, &writer.timer);

	ev_run(loop, 0);
EXIT:
	return rc;
}
