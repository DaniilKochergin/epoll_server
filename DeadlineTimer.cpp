#include <iostream>
#include "DeadlineTimer.h"

void DeadlineTimer::Add(int fd) {
    auto time = std::chrono::steady_clock::now() + std::chrono::seconds(SOCKET_WAIT_TIME);
    Timer[fd] = time;
    Queue.insert(std::make_pair(time, fd));
}

void DeadlineTimer::Remove(int fd) {
    Queue.erase(std::make_pair(Timer[fd], fd));
    Timer.erase(fd);
}

void DeadlineTimer::Update(int fd) {
    Queue.erase(std::make_pair(Timer[fd], fd));
    auto time = std::chrono::steady_clock::now() + std::chrono::seconds(SOCKET_WAIT_TIME);
    Timer[fd] = time;
    Queue.insert(std::make_pair(time, fd));
}

int DeadlineTimer::GetNextDeadline() {
    int res = 100;
    if (Queue.empty()) {
        return res;
    }
    auto it = Queue.begin();

    res = std::min(res, (int)std::chrono::duration_cast<std::chrono::milliseconds>(
            it->first - std::chrono::steady_clock::now()).count());
    return res;
}

std::vector<int> DeadlineTimer::GetOverdueFds() {
    std::vector<int> v;
    for (auto fd: Queue) {
        auto now = std::chrono::steady_clock::now();
        if (fd.first < now) {
            v.push_back(fd.second);
        } else {
            return v;
        }
    }
    return {};
}
