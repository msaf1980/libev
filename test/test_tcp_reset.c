/* read/write and timeout */
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <ev.h>

#define GO_LABEL(code, label)                                                  \
	do {                                                                       \
		rc = (code);                                                           \
		goto label;                                                            \
	} while (0);

int rc = 0;

typedef struct {
	ev_io watcher;
	ev_timer timer;
} io_t;

int ignore_sigpipe() {
	struct sigaction sa, osa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	return sigaction(SIGPIPE, &sa, &osa);
}

/*
 * sock_type - 0, SOCK_STREAM, SOCK_DGRAM
 * family    - AF_UNSPEC Allow IPv4 or IPv6,
 * return  getaddrinfo result code
 */
int getaddrinfo_all(const char *host, const char *service, int sock_type,
                    int protocol, int family, struct addrinfo *result) {
	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = family;
	hints.ai_socktype = sock_type;
	hints.ai_protocol = protocol;
	hints.ai_flags =
	    AI_PASSIVE; /* Don't return IPv6 addresses on loopback if IPv4 exists */

	return getaddrinfo(host, service, &hints, &result);
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
	if (rc == 0)
		rc = -1;
	/* close socket */
	close(client->watcher.fd);
}

static void connect_cb(EV_P_ ev_io *w, int revents) {
	io_t *client = (io_t *) (((char *) w) - offsetof(io_t, watcher));
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

int main(int argc, char *argv[]) {
	int fd;
	io_t client;

	int s;
	char *host = "127.0.0.1";
	char *service = "20150";
	struct addrinfo *address;

	ignore_sigpipe();

	if ((getaddrinfo_i(host, service, SOCK_STREAM, AF_UNSPEC, &address)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
		GO_LABEL(-1, EXIT);
	}
	if ((fd = socket(address->ai_family, address->ai_socktype,
	                 address->ai_protocol)) == -1) {
		fprintf(stderr, "connect failed. %s\n", strerror(errno));
		freeaddrinfo(address);
		GO_LABEL(-1, EXIT);
	}
	if (connect(fd, address->ai_addr, address->ai_addrlen) == -1) {
		fprintf(stderr, "connect failed. %s\n", strerror(errno));
		freeaddrinfo(address);
		GO_LABEL(-1, EXIT);
	}
	freeaddrinfo(address);

	set_nonblock(fd);

	return 0;

	rc = -1;

	struct ev_loop *loop = EV_DEFAULT;

	/* start watcher */
	ev_io_init(&client.watcher, connect_cb, fd, EV_WRITE);
	ev_io_start(loop, &client.watcher);

	/* initialise a timer watcher, then start non-repeating 5.0 second timeout
	 */
	ev_timer_init(&client.timer, timeout_cb, .2, 0.);
	ev_timer_start(loop, &client.timer);

	ev_run(loop, 0);
EXIT:
	return rc;
}
