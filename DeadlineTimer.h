#pragma once

#include <set>
#include <chrono>
#include <vector>
#include <unordered_map>

#define SOCKET_WAIT_TIME 1200

class DeadlineTimer {
public:
    typedef std::chrono::steady_clock::time_point TimePoint;

    DeadlineTimer() = default;

    void Update(int fd);

    void Remove(int fd);

    void Add(int fd);

    int GetNextDeadline();

    std::vector<int> GetOverdueFds();

private:
    std::set<std::pair<TimePoint, int>> Queue;
    std::unordered_map<int, TimePoint> Timer;
};