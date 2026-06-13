#pragma once

#include <chrono>
#include <string>

namespace CE::Utils {
    class ScopedTimer {
        public:
            ScopedTimer(std::string label);
            ~ScopedTimer();
        private:
            std::string mLabel;
            std::chrono::steady_clock::time_point mTimeStart;
    };
}