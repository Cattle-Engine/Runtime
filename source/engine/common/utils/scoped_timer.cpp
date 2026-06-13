#include "engine/common/utils/scoped_timer.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Utils {
    ScopedTimer::ScopedTimer(std::string label) {
        mTimeStart = std::chrono::steady_clock::now();
        mLabel = label;
    }

    ScopedTimer::~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> elapsed = end - mTimeStart;
        CE::Log(LogLevel::Debug, "{} took: {}", mLabel, elapsed.count());
    }
}