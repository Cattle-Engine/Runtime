#pragma once
#include <format>
#include <source_location>
#include <string>
#include <vector>
#include <utility>

namespace CECommon {
    enum class LogLevel {
        Debug,
        Info,
        Warn,
        Error, 
        Fatal,
    };

    void SetLogLevel(LogLevel level);
    const std::vector<std::pair<CECommon::LogLevel, std::string>>& GetLogs(); 
    void ClearLogs();

    void LogImpl(CECommon::LogLevel level, const std::string& message, std::source_location loc);

    inline void Log(CECommon::LogLevel level, const std::string& message,
                    std::source_location loc = std::source_location::current()) {
        LogImpl(level, message, loc);
    }
} // namespace CE

#define CE_LOG(level, ...) ::CECommon::LogImpl(level, std::format(__VA_ARGS__), std::source_location::current())