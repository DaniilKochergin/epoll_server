#include "timer.h"

timer::timer()
        : TimePoint(std::chrono::steady_clock::now()) {
}

int64_t timer::get_time() const noexcept {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - TimePoint).count();
}
