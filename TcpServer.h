#pragma once

class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();
    void Stop();

private:
    void listenSignal(int sig);
    void Loop();

private:
    const int server_fd;
    int event_fd{};
    int epollfd;
    volatile bool NeedToStop = false;
    struct sockaddr_in address{};
    static int const MAX_EVENTS = 20;
    struct epoll_event ev{}, events[MAX_EVENTS]{};
};