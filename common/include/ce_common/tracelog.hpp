#pragma once
#include <format>
#include <source_location>
#include <string>
#include <vector>
#include <utility>
#include <filesystem>

namespace CECommon {
    enum class LogLevel {
        Debug = 0,
        Info = 1,
        Warn = 2,
        Error = 3,
        Fatal = 4,
    };

    void SetLogLevel(LogLevel level);
    const std::vector<std::pair<CECommon::LogLevel, std::string>>& GetLogs(); 
    void ClearLogs();

    void LogImpl(CECommon::LogLevel level, const std::string& message, std::source_location loc);
    void Log(LogLevel level, const std::string& message, const std::string& file, int line);

    inline void Log(CECommon::LogLevel level, const std::string& message,
                    std::source_location loc = std::source_location::current()) {
        LogImpl(level, message, loc);
    }

    // returns false if it failed to either open the file path for write or write the actual contents of logs
    bool DumpLogsToFile(const std::filesystem::path& path);
} // namespace CE

#define CE_LOG(level, ...) ::CECommon::LogImpl(level, std::format(__VA_ARGS__), std::source_location::current())