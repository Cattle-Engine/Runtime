#pragma once
#include <string>
#include <format>

namespace CE {

enum LogLevel {
    Info,
    Warn,
    Debug,
    Error,
    Fatal,
};

void Log(LogLevel level, const std::string& message);

template<typename... Args>
void Log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    Log(level, message);   // calls the normal function
}

}