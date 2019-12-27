#pragma once

#include <chrono>

class timer {
public:
    timer();

    int64_t get_time() const noexcept;

private:
    std::chrono::steady_clock::time_point TimePoint;
};