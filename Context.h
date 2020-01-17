//
// Created by daniil on 28.12.2019.
//

#ifndef UNTITLED11_CONTEXT_H
#define UNTITLED11_CONTEXT_H

#include <atomic>
#include "DeadlineTimer.h"
#include "Communist.h"
#include <condition_variable>
#include <sys/epoll.h>
#include <netinet/in.h>


struct executable_task {
    virtual void operator()() = 0;

    virtual ~executable_task() = default;
};

class Context {
public:
    explicit Context();

    void Start();

    void AddEvent(sockaddr_in &addr, int fd, epoll_event &event, executable_task *task);

    int GetEpollfd();

    ~Context();

private:
    DeadlineTimer Timer;
    Communist Quota;

    std::atomic<bool> NeedToStop = false;
    const int server_fd;
    int signal_fd{};
    int epollfd;
    static int const MAX_EVENTS = 1024;
    struct epoll_event ev{}, events[MAX_EVENTS]{};
    std::unordered_map<int, executable_task *> tasks;
};


#endif //UNTITLED11_CONTEXT_H
