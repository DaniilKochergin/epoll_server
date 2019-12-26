#pragma once

#include "DeadlineTimer.h"
#include "Communist.h"
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <sys/epoll.h>

class TcpServer {
public:
    explicit TcpServer(int port);

    ~TcpServer();

    void AsyncStart();

    void BlockingStart();

    void Stop();

private:
    std::string ProcessRead(int n);

    static std::vector<std::string> SplitTextByCharacter(const std::string &s, char delim);

    void ProcessWrite(const std::string &buf, int n);

    void AddConnection();

    void ProcessSignalfd();

    void Loop();

private:
    std::atomic<bool> NeedToStop = false;
    bool Quit = false;
    DeadlineTimer Timer;
    Communist Quota;

    std::mutex m;
    std::condition_variable Stopped;
    const int server_fd;
    int signal_fd{};
    int epollfd;
    struct sockaddr_in address{};
    static int const MAX_EVENTS = 1024;
    struct epoll_event ev{}, events[MAX_EVENTS]{};
};