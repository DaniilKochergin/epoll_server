#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <future>
#include <zconf.h>
#include "TcpServer.h"
#include <signal.h>
#include <iostream>
#include <sys/signalfd.h>
#include <netdb.h>
#include <string>
#include <iostream>

TcpServer::TcpServer(int port) :
        server_fd(socket(AF_INET, SOCK_STREAM, 0)),
        signal_fd(0) {

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);


    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address))) {
        throw std::runtime_error("Can't create server, bind failed\n");
    }

    if (listen(server_fd, 3)) {
        throw std::runtime_error("Can't start listening server\n");
    }

    if ((epollfd = epoll_create1(0)) == -1) {
        throw std::runtime_error("Can't create epoll");
    }
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        throw std::runtime_error("Can't create server_fd\n");
    }

    sigprocmask(SIG_BLOCK, &mask, nullptr);

    if ((signal_fd = signalfd(-1, &mask, 0)) == -1) {
        throw std::runtime_error("Can't create server, signalfd failed\n");
    }

    ev.events = EPOLLIN;
    ev.data.fd = signal_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, signal_fd, &ev)) {
        throw std::runtime_error("Can't create signals handler\n");
    }

}

void TcpServer::Loop() {
    for (;;) {
        char buffer[1024] = {0};
        int addrlen = sizeof(address);
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                int conn_sock = accept(server_fd, (struct sockaddr *) &address,
                                       (socklen_t *) &addrlen);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev)) {
                    std::cout << "New connection failed\n";
                } else {
                    std::cout << "New connection succesfuly added\n";
                }
            } else if (events[n].data.fd == signal_fd) {
                struct signalfd_siginfo info;
                if (!read(signal_fd, &info, sizeof(info))) {
                    std::cout << "Can't read " << signal_fd;
                    continue;
                }

                unsigned sig = info.ssi_signo;
                unsigned user = info.ssi_uid;
                if (sig == SIGINT) {
                    std::cout << "Got SIGINT from user " << user << '\n';
                    NeedToStop = true;
                }
                if (sig == SIGTERM) {
                    std::cout << "Got SIGTERM from user " << user << '\n';
                    NeedToStop = true;
                }
            } else {
                struct addrinfo hints, *infoptr;
                hints.ai_family = AF_INET;
                int len = read(events[n].data.fd, buffer, 1024);
                if (len == -1) {
                    std::cout << "Can't read " << signal_fd << '\n';
                    continue;
                }
                std::cout << buffer << std::endl;
                std::string buf;
                try {
                    buf = std::string(buffer, len - 2);
                } catch (...) {
                    buf = "";
                }
                if (int res = getaddrinfo(buf.data(), NULL, &hints, &infoptr)) {
                    std::string error = gai_strerror(res);
                    error.push_back('\n');
                    write(events[n].data.fd, error.c_str(), error.size());
                    continue;
                }
                struct addrinfo *p;
                std::string s;

                for (p = infoptr; p != NULL; p = p->ai_next) {
                    char host[256];
                    if (int res = getnameinfo(p->ai_addr, p->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST)) {
                        std::string err = gai_strerror(res);
                        s += '\n';
                        if (write(events[n].data.fd, err.c_str(), err.size()) == -1) {
                            std::cout << "Write failed\n";
                            break;
                        }
                    }
                    s += host;
                    s += "\n";
                }

                if (write(events[n].data.fd, s.c_str(), s.size()) == -1) {
                    std::cout << "Write failed\n";
                }

                freeaddrinfo(infoptr);
            }
        }
        if (NeedToStop) {
            return;
        }
    }

}

TcpServer::~TcpServer() {
    //  NeedToStop = true;
}

void TcpServer::Stop() {
    NeedToStop = true;
}

void TcpServer::Start() {
    auto thr = std::thread([this]() { this->Loop(); });
    thr.join();
}
