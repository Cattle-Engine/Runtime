#include "ce_common/tracelog.hpp"
#include "ce_common/platforms.hpp"

#include <chrono>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <format>
#include <source_location>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    CECommon::LogLevel gLogLevel =
    #ifdef CE_DEBUG
            CECommon::LogLevel::Debug;
    #else
            CECommon::LogLevel::Info;
    #endif
    using Manip = std::ostream& (*)(std::ostream&);

    std::string gLoggingPrefix;
    std::vector<std::pair<CECommon::LogLevel, std::string>> gLogs;
} // namespace

namespace CECommon {
    inline bool g_UseANSI = Platforms::EnableANSI();

    inline std::ostream& reset(std::ostream& os) {
        if (g_UseANSI)
            os << "\x1b[0m";
        return os;
    }

    inline std::ostream& red(std::ostream& os) {
        if (g_UseANSI)
            os << "\x1b[31m";
        return os;
    }

    inline std::ostream& yellow(std::ostream& os) {
        if (g_UseANSI)
            os << "\x1b[33m";
        return os;
    }

    inline std::ostream& blue(std::ostream& os) {
        if (g_UseANSI)
            os << "\x1b[34m";
        return os;
    }

    inline std::ostream& bold_red(std::ostream& os) {
        if (g_UseANSI)
            os << "\x1b[1;31m";
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

    std::string GetLocation(const std::source_location& loc, LogLevel level) {
        std::string_view file = loc.file_name();

        size_t source_pos = file.rfind("source/");
        size_t include_pos = file.rfind("include/");

        size_t pos = std::string_view::npos;

        if (source_pos != std::string_view::npos)
            pos = source_pos;
        if (include_pos != std::string_view::npos)
            pos = (pos == std::string_view::npos) ? include_pos : std::max(pos, include_pos);

        if (pos != std::string_view::npos)
            file.remove_prefix(pos);

        if (gLogLevel == LogLevel::Debug)
            return std::format("[{}:{}] ", file, loc.line());

        return {};
    }

    void SetLogLevel(LogLevel level) {
        gLogLevel = level;
    }

    const std::vector<std::pair<CECommon::LogLevel, std::string>>& GetLogs() {
        return gLogs;
    }

    void ClearLogs() {
        gLogs.clear();
    }

    void LogImpl(LogLevel level, const std::string& message, std::source_location loc) {
        if (static_cast<int>(level) < static_cast<int>(gLogLevel)) return;

        std::ostream& os = std::cout;
        Manip colour = nullptr;
        std::string tag;

        switch (level) {
        case LogLevel::Info:
            colour = blue;
            tag = "[INFO]";
            break;
        case LogLevel::Warn:
            colour = yellow;
            tag = "[WARNING]";
            break;
        case LogLevel::Debug:
            colour = blue;
            tag = "[DEBUG]";
            break;
        case LogLevel::Error:
            colour = red;
            tag = "[ERROR]";
            break;
        case LogLevel::Fatal:
            colour = bold_red;
            tag = "[FATAL]";
            break;
        }
        
        std::string log_prefix;
        if (!gLoggingPrefix.empty()) {
            log_prefix = " [" + gLoggingPrefix + "]";
        }

        std::string formatted_message =
            "[" + GetTimestamp() + "] " + tag +
            log_prefix + " " + GetLocation(loc, level) + message;

        gLogs.emplace_back(level, formatted_message);

        if (colour)
            os << colour;

        os << formatted_message << reset << std::endl;
    }

    void Log(LogLevel level, const std::string& message, const std::string& file, int line) {
        if (static_cast<int>(level) < static_cast<int>(gLogLevel)) return;

        std::ostream& os = std::cout;
        Manip colour = nullptr;
        std::string tag;

        switch (level) {
        case LogLevel::Info:
            colour = blue;
            tag = "[INFO]";
            break;
        case LogLevel::Warn:
            colour = yellow;
            tag = "[WARNING]";
            break;
        case LogLevel::Debug:
            colour = blue;
            tag = "[DEBUG]";
            break;
        case LogLevel::Error:
            colour = red;
            tag = "[ERROR]";
            break;
        case LogLevel::Fatal:
            colour = bold_red;
            tag = "[FATAL]";
            break;
        }
        
        std::string log_prefix;
        if (!gLoggingPrefix.empty()) {
            log_prefix = " [" + gLoggingPrefix + "]";
        }

        std::string formatted_message =
            "[" + GetTimestamp() + "] " + tag +
            log_prefix + " " + std::format("[{}:{}]", file, line) + message;

        gLogs.emplace_back(level, formatted_message);

        if (colour)
            os << colour;

        os << formatted_message << reset << std::endl;
    }

    bool DumpLogsToFile(const std::filesystem::path& path) {
        try {
            std::ofstream file(path, std::ios::out | std::ios::trunc);

            if (!file.is_open())
                return false;

            for (const auto& [level, message] : gLogs)
                file << message << '\n';

            file.flush();

            if (!file.good())
                return false;

            file.close();

            return !file.fail();
        }
        catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }
} // namespace CECommon