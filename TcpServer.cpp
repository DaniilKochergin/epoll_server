#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <future>
#include <zconf.h>
#include "TcpServer.h"
#include <signal.h>
#include <iostream>
#include <sys/signalfd.h>

TcpServer::TcpServer(int port) :
        server_fd(socket(AF_INET, SOCK_STREAM, 0)),
        event_fd(0) {

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);


    bind(server_fd, (struct sockaddr *) &address, sizeof(address));

    listen(server_fd, 3);

    epollfd = epoll_create1(0);

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        std::cout<<"can't create server_fd"<<std::endl;
    }
    sigprocmask(SIG_BLOCK, &mask, NULL);

    event_fd = signalfd(-1, &mask, 0);

    ev.events = EPOLLIN;
    ev.data.fd = event_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, event_fd, &ev)) {
        std::cout<<"can't create signals handler:(";
    }

    auto lambda = [this]() { this->Loop(); };
//    lambda();

    auto thr = std::thread(lambda);
    thr.join();

}

void TcpServer::Loop() {
    for (;;) {
        char buffer[1024] = {0};
        int addrlen = sizeof(address);
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        for (int n = 0; n < nfds; ++n) {
            std::cout<<events[n].data.fd<<std::endl;
            if (events[n].data.fd == server_fd) {
                int conn_sock = accept(server_fd, (struct sockaddr *) &address,
                                       (socklen_t *) &addrlen);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev);
            } else if (events[n].data.fd == event_fd) {
                struct signalfd_siginfo info;
                read(event_fd, &info, sizeof(info));

                unsigned sig = info.ssi_signo;
                unsigned user = info.ssi_uid;
                std::cout<<sig<<std::endl;
                if (sig == SIGINT) {
                    printf("Got SIGINT from user %u\n", user);
                    NeedToStop = true;
                }
                if (sig == SIGTERM) {
                    printf("Got SIGTERM from user %u\n", user);
                    NeedToStop = true;
                }
            } else {
                int len = read(events[n].data.fd, buffer, 1024);
                write(events[n].data.fd, buffer, len);
            }
        }
        if (NeedToStop) {
            return;
        }
        //   std::this_thread::sleep_for(std::chrono::seconds(1));
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

void TcpServer::listenSignal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        NeedToStop = true;
    } else {
        exit(0);
    }
}