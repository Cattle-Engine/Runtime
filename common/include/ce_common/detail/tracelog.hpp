#pragma once
#include <format>
#include <source_location>
#include <string>

namespace CECommon {
    enum class LogLevel {
        Info,
        Warn,
        Debug,
        Error, 
        Fatal,
    };

    void LogImpl(LogLevel level, const std::string& message, std::source_location loc);

    inline void Log(LogLevel level, const std::string& message,
                    std::source_location loc = std::source_location::current()) {
        LogImpl(level, message, loc);
    }
} // namespace CE

#define CE_LOG(level, ...) ::CECommon::LogImpl(level, std::format(__VA_ARGS__), std::source_location::current())