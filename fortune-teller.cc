// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Simple fortune teller: on connect, the client gets a line of fortune.

#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>
#include <string>

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

int main(int, char *[]) {
    signal(SIGPIPE, SIG_IGN);  // Pesky clients, closing connections...

    static constexpr int kPort = 3001;

    // Random things from /usr/share/games/fortunes/fortunes
    static std::vector<std::string> fortunes = {
        "There is a fly on your nose.\n",
        "Today is the first day of the rest of your life.\n",
        "Today is what happened to yesterday.\n",
        "Tomorrow will be cancelled due to lack of interest.\n",
        "So you're back... about time...\n",
        "Look afar and see the end from the beginning.\n",
        "Is that really YOU that is reading this?\n",
        "Don't Worry, Be Happy.\n",
    };

    FDMultiplexer *const fmux = new FDMultiplexer();
    const int listen_socket = create_bound_socket(kPort);
    if (listen_socket < 0)
        return 1;

    if (listen(listen_socket, 2) < 0) {
        perror("Couldn't listen\n");
        return 1;
    }

    fprintf(stderr, "Listening on %d\nQuery e.g. with\n\tnc localhost %d\n",
            kPort, kPort);

    int next_fortune = 0;

    // 'Readable' on a listen socket is if there is a new connection waiting.
    fmux->RunOnReadable(listen_socket, [listen_socket, &next_fortune]() {
        const int fd = accept(listen_socket, nullptr, nullptr);
        if (fd < 0) return true;
        const std::string &fortune = fortunes[next_fortune];
        write(fd, fortune.c_str(), fortune.length());
        next_fortune = (next_fortune + 1) % fortunes.size();
        close(fd);
        return true;
    });

    fmux->Loop();
}
