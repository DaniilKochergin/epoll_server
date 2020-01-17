#include <csignal>
#include <sys/signalfd.h>
#include <zconf.h>
#include <iostream>
#include "Context.h"
#include "timer.h"

#define MAX_SOCK_LISTEN 1024

Context::Context() : server_fd(socket(AF_INET, SOCK_STREAM, 0)),
                     signal_fd(0) {

    if ((epollfd = epoll_create1(0)) == -1) {
        throw std::runtime_error("Can't create epoll");
    }
}

void Context::Start() {
    for (;;) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, Timer.GetNextDeadline());
        timer clock;
        for (int n = 0; n < nfds; ++n) {
            Timer.Update(events[n].data.fd);
            if (Quota.HaveQuotaToEvent(events[n].data.fd)) {
                auto time = std::chrono::steady_clock::now();
                (*static_cast<executable_task *>(events[n].data.ptr))();
                Quota.Update(events[n].data.fd, std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - time).count());
            }

            if (NeedToStop) {
                return;
            }
        }

        for (const auto fd : Timer.GetOverdueFds()) {
            Timer.Remove(fd);
            Quota.Remove(fd);
            delete tasks[fd];
            tasks.erase(fd);
            std::cout << "Removed fd:" << fd << std::endl;
        }

        Quota.AddQuotaToAll(std::max(clock.get_time() / std::max((int) Quota.GetCountFd(), 1), (int64_t) 1));

        if (NeedToStop) {
            return;
        }

    }
}

void Context::AddEvent(sockaddr_in &addr, int fd, epoll_event &event, executable_task *task) {
    event.data.ptr = task;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event)) {
        std::cout << "New connection failed\n";
        delete static_cast<executable_task *>(event.data.ptr);
        return;
    } else {
        std::cout << "New connection succesfuly added\n";
    }
    Timer.Add(fd);
    Quota.Add(addr.sin_addr.s_addr, fd);
    tasks[fd] = static_cast<executable_task *>(task);
}

int Context::GetEpollfd() {
    return epollfd;
}

Context::~Context() {
    NeedToStop = true;
}
