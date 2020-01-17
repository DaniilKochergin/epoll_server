#pragma once

#include "DeadlineTimer.h"
#include "Communist.h"
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <sys/epoll.h>
#include "Context.h"

class TcpServer : executable_task {
public:
    explicit TcpServer(Context *context, int port);

    ~TcpServer() override;

    void operator()() override;

private:

    struct task : executable_task {
        explicit task(int fd);

        ~task() override;

        void operator()() override;

    private:
        std::string ProcessRead();

        void ProcessWrite(const std::string &s);

        std::vector<std::string> SplitTextByCharacter(const std::string &s, char delim);

    private:
        int fd;
        std::string buf;
    };

private:
    Context *context;
    struct sockaddr_in address{};
    const int server_fd;
    struct epoll_event ev{};
};