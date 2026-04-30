#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <ostream>

#include "engine/platforms.hpp"
#include "engine/common/tracelog.hpp"

namespace CE {

    inline bool g_UseANSI = Platforms::EnableANSI();

    inline std::ostream& reset(std::ostream& os) {
        if (g_UseANSI) os << "\x1b[0m";
        return os;
    }

    inline std::ostream& red(std::ostream& os) {
        if (g_UseANSI) os << "\x1b[31m";
        return os;
    }

    inline std::ostream& yellow(std::ostream& os) {
        if (g_UseANSI) os << "\x1b[33m";
        return os;
    }

    inline std::ostream& blue(std::ostream& os) {
        if (g_UseANSI) os << "\x1b[34m";
        return os;
    }

    inline std::ostream& bold_red(std::ostream& os) {
        if (g_UseANSI) os << "\x1b[1;31m";
        return os;
    }

    inline std::ostream& endl(std::ostream& os) {
        if (g_UseANSI) os << "\x1b[0m";
        os << '\n';
        return os;
    }

    std::string GetTimestamp() {
        using namespace std::chrono;

        auto now = system_clock::now();
        std::time_t now_time = system_clock::to_time_t(now);

        std::tm tm{};
        #ifdef _WIN32
        localtime_s(&tm, &now_time);
        #else
        localtime_r(&now_time, &tm);
        #endif

        std::ostringstream ss;
        ss << std::put_time(&tm, "%H:%M:%S");
        return ss.str();
    }

    void Log(LogLevel level, const std::string& message) {
        std::ostream& os = std::cout;

        switch (level) {
            case Info:
                os << blue
                   << "[" << GetTimestamp() << "] [INFO] " << message;
                break;

            case Warn:
                os << yellow
                   << "[" << GetTimestamp() << "] [WARN] " << message;
                break;

            case Debug:
                os << blue
                   << "[" << GetTimestamp() << "] [DEBUG] " << message;
                break;

            case Error:
                os << red
                   << "[" << GetTimestamp() << "] [ERROR] " << message;
                break;

            case Fatal:
                os << bold_red
                   << "[" << GetTimestamp() << "] [FATAL] " << message;
                break;
        }

        os << std::endl;
    }
}