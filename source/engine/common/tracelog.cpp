#include <iostream>
#include "engine/common/tracelog.hpp"

#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_BOLD_RED "\033[1;31m"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

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

namespace CE {
    void Log(LogLevel level, const std::string& message) {
        switch (level) {
            case Info:
                std::cout << ANSI_BLUE << "[" << GetTimestamp() << "] [INFO] " << message << ANSI_RESET << std::endl;
                break;
            case Warn:
                std::cout << ANSI_YELLOW << "[" << GetTimestamp() << "] [WARN] " << message << ANSI_RESET << std::endl;
                break;
            case Debug:
                std::cout << ANSI_BLUE << "[" << GetTimestamp() << "] [DEBUG] " << message << ANSI_RESET << std::endl;
                break;
            case Error:
                std::cout << ANSI_RED << "[" << GetTimestamp() << "] [ERROR] " << message << ANSI_RESET << std::endl;
                break;
            case Fatal:
                std::cout << ANSI_BOLD_RED << "[" << GetTimestamp() << "] [FATAL] " << message << ANSI_RESET << std::endl;
                break;
        }
    }
}