#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <future>
#include <zconf.h>
#include "TcpServer.h"

TcpServer::TcpServer(int port) :
        server_fd(socket(AF_INET, SOCK_STREAM, 0)) {

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *) &address, sizeof(address));

    listen(server_fd, 3);

    epollfd = epoll_create1(0);

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev);

    auto lambda = [this]() { this->Loop(); };
//    lambda();

    auto thr = std::thread(lambda);
    thr.detach();

}

void TcpServer::Loop() {
    for (;;) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        char buffer[1024] = {0};
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                int addrlen = sizeof(address);
                int conn_sock = accept(server_fd, (struct sockaddr *) &address,
                                   (socklen_t *) &addrlen);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev);
//                int len = read(events[n].data.fd, buffer, 1024);
//                write(events[n].data.fd, buffer, len);
            } else {
                int len = read(events[n].data.fd, buffer, 1024);
                write(events[n].data.fd, buffer, len);
            }
        }
        if (NeedToStop) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
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
