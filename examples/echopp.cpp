/* Copyright 2017 evan */
#include <netinet/in.h>
#include <sys/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <resolv.h>
#include <stdlib.h>
#include <string.h>

#include <list>

#include <ev++.h>

// Simply adds O_NONBLOCK to the file descriptor of choice
int setnonblock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

struct Buffer {
  char *data;
  ssize_t len;
  ssize_t pos;
  // Buffer is char array
  Buffer(const char *bytes, ssize_t nbytes) {
    pos = 0;
    len = nbytes;
    data = new char[nbytes];
    memcpy(data, bytes, nbytes);
  }
  virtual ~Buffer() { delete[] data; }

  char *dpos() { return data + pos; }

  ssize_t nbytes() { return len - pos; }
};

class EchoInstance {
private:
  ev::io io;
  static int total_clients;
  int sfd;

  std::list<Buffer *> write_queue;

public:
  void callback(ev::io &wather, int revents) {
    if (EV_ERROR & revents) {
      perror("got invalid event");
      close();
      return;
    }

    if (revents & EV_READ) {
      this->read_cb(wather);
    }

    if (revents & EV_WRITE) {
      this->write_cb(wather);
    }

    if (this->write_queue.empty()) {
      io.set(ev::READ);
    } else {
      io.set(ev::READ | ev::WRITE);
    }
  }

  void read_cb(const ev::io &watcher) {
    char buffer[1024];
    ssize_t nread = recv(watcher.fd, buffer, sizeof(buffer), 0);

    if (nread < 0) {
      perror("read error");
      return;
    }

    if (nread == 0) {
      // after close delete initited
      this->close();
    } else {
      this->write_queue.push_back(new Buffer(buffer, nread));
    }
  }

  void write_cb(const ev::io &watcher) {
    if (this->write_queue.empty()) {
      io.set(ev::READ);
      return;
    }

    Buffer *buffer = this->write_queue.front();

    ssize_t written = write(watcher.fd, buffer->dpos(), buffer->nbytes());
    if (written < 0) {
      perror("write error");
      return;
    }

    buffer->pos += written;
    if (buffer->nbytes() == 0) {
      this->write_queue.pop_front();
      delete buffer;
    }
  }

  virtual ~EchoInstance() {
    printf("%d clients(s) connected. \n", --this->total_clients);

    // stop and free watcher if client socket is closing
    close();
  }

  void init() {
    printf("get connection %d\n", this->total_clients++);

    this->io.set<EchoInstance, &EchoInstance::callback>(this);
    // this->io.set(this);
    this->io.start(this->sfd, ev::READ);
  }

  void close() {
    this->io.stop();
    if (this->sfd != -1) {
      ::close(this->sfd);
      this->sfd = -1;
    }
  }

public:
  explicit EchoInstance(int s) : sfd(s) {
    setnonblock(s);
    this->init();
  }
};

class EchoServer {
private:
  ev::io io;
  ev::sig sio;
  int s;

public:
  explicit EchoServer(int port) {
    printf("Listening on port %d\n", port);
    struct sockaddr_in addr;

    s = socket(PF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
      perror("bind");
    }

    setnonblock(s);
    listen(s, 5);

    io.set<EchoServer, &EchoServer::io_accept>(this);
    io.start(s, ev::READ);

    sio.set<&EchoServer::signal_cb>();
    sio.start(SIGINT);
  }

  ~EchoServer() {
    shutdown(s, SHUT_RDWR);
    close(s);
  }

  void io_accept(ev::io &watcher, int revents) {
    if (EV_ERROR & revents) {
      perror("got invalid event");
      // return;
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_sd =
        accept(watcher.fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_sd < 0) {
      perror("accept error");
      return;
    }

    EchoInstance *client = new EchoInstance(client_sd);
  }

  static void signal_cb(ev::sig &signal, int revents) {
    signal.loop.break_loop();
  }
};

int EchoInstance::total_clients = 0;

int main(int argc, char **argv) {
  int port = 8192;

  if (argc > 1) {
    port = atoi(argv[1]);
  }

  ev::default_loop loop;
  EchoServer echo(port);

  // run
  loop.run(0);
  return 0;
}
