#include "engine/common/utils/scoped_timer.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Utils {
    ScopedTimer::ScopedTimer(std::string label) {
        mTimeStart = std::chrono::steady_clock::now();
        mLabel = label;
    }
    
    ScopedTimer::~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();

        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - mTimeStart).count();

        if (ns >= 1'000'000'000) {
            CE::Log(LogLevel::Debug,
                "{} took: {:.3f} s",
                mLabel,
                ns / 1e9);
        } else if (ns >= 1'000'000) {
            CE::Log(LogLevel::Debug,
                "{} took: {:.3f} ms",
                mLabel,
                ns / 1e6);
        } else if (ns >= 1'000) {
            CE::Log(LogLevel::Debug,
                "{} took: {:.3f} us",
                mLabel,
                ns / 1e3);
        } else {
            CE::Log(LogLevel::Debug,
                "{} took: {} ns",
                mLabel,
                ns);
        }
    }
}