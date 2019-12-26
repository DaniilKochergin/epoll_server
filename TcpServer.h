#pragma once

#include "DeadlineTimer.h"
#include "Communist.h"

class TcpServer {
public:
    explicit TcpServer(int port);

    ~TcpServer();

    void Stop();

    void Start();

private:
    std::string ProcessRead(int n);

    static std::vector<std::string> SplitTextByCharacter(const std::string &s, char delim);

    void ProcessWrite(const std::string &buf, int n);

    void AddConnection();

    void ProcessSignalfd();

    void Loop();

private:
    volatile bool NeedToStop = false;
    DeadlineTimer Timer;
    Communist Quota;

    const int server_fd;
    int signal_fd{};
    int epollfd;
    struct sockaddr_in address{};
    static int const MAX_EVENTS = 1024;
    struct epoll_event ev{}, events[MAX_EVENTS]{};
};