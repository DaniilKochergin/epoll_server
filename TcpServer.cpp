#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <future>
#include <zconf.h>
#include "TcpServer.h"
#include "timer.h"
#include <csignal>
#include <iostream>
#include <sys/signalfd.h>
#include <netdb.h>
#include <string>
#include <sstream>

#define MAX_SOCK_LISTEN 1024

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

    if (listen(server_fd, MAX_SOCK_LISTEN)) {
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
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, Timer.GetNextDeadline());
        timer clock;
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                AddConnection();
            } else if (events[n].data.fd == signal_fd) {
                ProcessSignalfd();
            } else {
                Timer.Update(events[n].data.fd);
                if (Quota.HaveQuotaToEvent(events[n].data.fd)) {
                    auto time = std::chrono::steady_clock::now();
                    try {
                        auto buf = ProcessRead(n);
                        ProcessWrite(buf, n);
                    } catch (const std::runtime_error &e) {
                        std::cout << e.what() << '\n';
                    }
                    Quota.Update(events[n].data.fd, std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - time).count());
                } else {
                    ProcessRead(n);
                    std::string retry =
                            "Quota is over, retry via " + std::to_string(-Quota.GetQuota(events[n].data.fd)) + " ms";
                    write(events[n].data.fd, retry.c_str(), retry.size());
                }
            }

            if (NeedToStop) {
                return;
            }
        }


        for (const auto fd : Timer.GetOverdueFds()) {
            close(fd);
            Timer.Remove(fd);
            Quota.Remove(fd);
        }

        int64_t res = clock.get_time();
        int64_t count = Quota.GetCountFd();
        std::cout << res << " " << Quota.GetCountFd() << std::endl;
        Quota.AddQuotaToAll(std::max(clock.get_time() / std::max((int) Quota.GetCountFd(), 1), (int64_t) 1));


        if (NeedToStop) {
            return;
        }

    }
}

TcpServer::~TcpServer() {
    close(signal_fd);
    close(server_fd);
}

void TcpServer::Stop() {
    NeedToStop = true;
}

void TcpServer::Start() {
    auto thr = std::thread([this]() { this->Loop(); });
    thr.join();
}

std::string TcpServer::ProcessRead(int n) {
    char buffer[1024];
    int len = read(events[n].data.fd, buffer, 1024);
    if (len == -1) {
        throw std::runtime_error("Read failed");
    }
    std::string buf;
    try {
        buf = std::string(buffer, len);
    } catch (...) {
        buf = "";
    }
    std::cout << "Message from " << events[n].data.fd << ": \"" << buf << "\"\n";
    return buf;
}

void TcpServer::ProcessWrite(const std::string &text, int n) {
    for (const auto &buf : SplitTextByCharacter(text, '\n')) {
        struct addrinfo hints, *infoptr;
        hints.ai_family = AF_INET;
        if (int res = getaddrinfo(buf.data(), NULL, &hints, &infoptr)) {
            std::string error = gai_strerror(res);
            error.push_back('\n');
            write(events[n].data.fd, error.c_str(), error.size());
            return;
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

        s += "\n";

        if (write(events[n].data.fd, s.c_str(), s.size()) == -1) {
            std::cout << "Write failed\n";
        }

        freeaddrinfo(infoptr);
    }
}

void TcpServer::ProcessSignalfd() {
    struct signalfd_siginfo info;
    if (!read(signal_fd, &info, sizeof(info))) {
        std::cout << "Can't read signalfd" << signal_fd;
        return;
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
}

void TcpServer::AddConnection() {
    int addrlen = sizeof(address);
    int conn_sock = accept(server_fd, (struct sockaddr *) &address,
                           (socklen_t *) &addrlen);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = conn_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev)) {
        std::cout << "New connection failed\n";
    } else {
        std::cout << "New connection succesfuly added\n";
        Timer.Add(conn_sock);
        Quota.Add(address.sin_addr.s_addr, conn_sock);
    }
}

std::vector<std::string> TcpServer::SplitTextByCharacter(const std::string &s, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delim)) {
        while (!token.empty() && (token.back() == '\r' || token.back() == '\n')) {
            token.pop_back();
        }
        tokens.push_back(token);
    }
    return tokens;
}
