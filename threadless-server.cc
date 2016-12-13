// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Simple test for multiplexing server, handling connections in parallel.
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "fd-mux.h"

static int create_bound_socket(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("creating socket()");
        return -1;
    }

    struct sockaddr_in serv_addr = {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    int on = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (bind(s, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Trouble bind()-ing");
        return -1;
    }
    return s;
}

// Typical way to handle a connection. Usually a connection is longer lived
// and deals with multiple requests/responses, keeping state between them.
// This simulates that.
class EchoConnection {
public:
    EchoConnection(int fd, FDMultiplexer *fmux) : fd_(fd), count_(0) {
        if (fd < 0) { delete this; return; }  // Got called with invalid fd.
        fprintf(stderr, "New connection on fd %d\n", fd_);
        dprintf(fd_, "Hello! Any text will be echoed. 'exit' quits.\n");
        // Whenever there is something to read in the future on this
        // filedescriptor, deal with it in the HandleRead() method.
        // Tell the FD multiplexer to call us back when this happens.
        fmux->RunOnReadable(fd, [this](){ return HandleRead(); });
    }

private:
    bool HandleRead() {
        int r = read(fd_, buffer_, sizeof(buffer_));
        if (r <= 0)
            return FinishConnection();
        while (r > 0 && isspace(buffer_[r-1]))  // get rid of \r\n
            --r;
        buffer_[r] = '\0';
        dprintf(fd_, "%d. %s\n", ++count_, buffer_);
        if (strncmp(buffer_, "exit", 4) == 0) {
            return FinishConnection();
        }
        return true;
    }

    bool FinishConnection() {
        dprintf(fd_, "Thanks, have a good day. This was %d lines.\n", count_);
        close(fd_);
        delete this;
        return false;
    }

    const int fd_;
    int count_;
    char buffer_[1024];
};

int main(int, char *[]) {
    signal(SIGPIPE, SIG_IGN);  // Pesky clients, closing connections...

    FDMultiplexer *fmux = new FDMultiplexer();
    int listen_socket = create_bound_socket(3000);
    if (listen_socket < 0)
        return 1;

    if (listen(listen_socket, 2) < 0) {
        perror("Couldn't listen\n");
        return 1;
    }

    // 'Readable' on a listen socket is if there is a new connection waiting.
    // So we spawn a new connection which handles reads until it is done.
    fmux->RunOnReadable(listen_socket, [listen_socket,fmux]() {
            // Will delete itself when done.
            new EchoConnection(accept(listen_socket, nullptr, nullptr), fmux);
            return true;  // Keep accepting connections.
        });

    fmux->Loop();
}
