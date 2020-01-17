#include <sys/socket.h>
#include <sys/epoll.h>
#include <future>
#include <zconf.h>
#include "TcpServer.h"
#include <iostream>
#include <netdb.h>
#include <string>
#include <sstream>
#include <memory>

#define MAX_SOCK_LISTEN 1000000
#define MAX_BUF_SIZE 10000

TcpServer::TcpServer(Context *context, int port) :
        context(context),
        server_fd(socket(AF_INET, SOCK_STREAM, 0)) {

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);


    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address))) {
        throw std::runtime_error("Can't create server, bind failed\n");
    }

    if (listen(server_fd, MAX_SOCK_LISTEN)) {
        throw std::runtime_error("Can't start listening server\n");
    }


    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    context->AddEvent(address, server_fd, ev, this);


}

TcpServer::~TcpServer() {
    close(server_fd);
}


void TcpServer::operator()() {
    int addrlen = sizeof(address);
    int conn_sock = accept(server_fd, (struct sockaddr *) &address,
                           (socklen_t *) &addrlen);
    if (conn_sock == -1) {
        std::cout << "Accept failed";
        return;
    }
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = conn_sock;
    context->AddEvent(address, conn_sock, ev, new task(conn_sock));
}

std::string TcpServer::task::ProcessRead() {
    char buffer[1024];
    int len = read(fd, buffer, 1024);
    if (len < -1) {
        throw std::runtime_error("Read failed");
    }
    if (len == 0) {
        return "";
    }
    std::string s;
    s = std::string(buffer, len);
    return s;
}

void TcpServer::task::ProcessWrite(const std::string &s) {
    for (const auto &text : SplitTextByCharacter(s, '\n')) {
        struct addrinfo hints{}, *infoptr = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_flags = 0;
        hints.ai_socktype = SOCK_STREAM;
        if (int res = getaddrinfo(text.data(), NULL, &hints, &infoptr)) {
            std::string error = gai_strerror(res);
            error.push_back('\n');
            write(fd, error.c_str(), error.size());
            freeaddrinfo(infoptr);
            return;
        }
        struct addrinfo *p;
        std::string tmp;

        for (p = infoptr; p != NULL; p = p->ai_next) {
            char host[256];
            if (int res = getnameinfo(p->ai_addr, p->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST)) {
                std::string err = gai_strerror(res);
                tmp += '\n';
                if (write(fd, err.c_str(), err.size()) == -1) {
                    std::cout << "Write failed\n";
                    break;
                }
            }
            tmp += host;
            tmp += "\n";
        }

        tmp += "\n";

        if (write(fd, tmp.c_str(), tmp.size()) == -1) {
            std::cout << "Write failed\n";
        }

        freeaddrinfo(infoptr);
    }
}

TcpServer::task::task(int fd) : fd(fd) {
}

void TcpServer::task::operator()() {
    try {
        auto s = ProcessRead();
        ProcessWrite(s);
    } catch (const std::runtime_error &e) {
        std::cout << e.what() << '\n';
    }
}

std::vector<std::string> TcpServer::task::SplitTextByCharacter(const std::string &s, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    bool is_first = true;
    while (std::getline(tokenStream, token, delim)) {
        while (!token.empty() && token.back() == '\r') {
            token.pop_back();
        }
        if (is_first) {
            tokens.push_back(buf + token);
            is_first = false;
        }
    }
    if (s.back() != '\n' && s.back() != '\r' && !tokens.empty()) {
        buf = tokens.back();
        tokens.pop_back();
    } else {
        buf.clear();
    }

    if (buf.size() > MAX_BUF_SIZE) {
        tokens.push_back(buf);
        buf.clear();
    }
    return tokens;
}

TcpServer::task::~task() {
    close(fd);
}
