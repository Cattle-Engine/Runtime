#include "ce_common/detail/tracelog.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

namespace {
    bool kIsDebug =
#if defined(CE_DEBUG)
        true;
#else
        false;
#endif
    using Manip = std::ostream& (*)(std::ostream&);
} // namespace

namespace CECommon {
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

    std::string GetLocation(const std::source_location& loc) {
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

        if (kIsDebug)
            return std::format("[{}:{}] ", file, loc.line());

        return {};
    }

    void LogImpl(LogLevel level, const std::string& message, std::source_location loc) {
        if (level == LogLevel::Debug && !kIsDebug)
            return;

        std::ostream& os = std::cout;
        std::string tag;

        switch (level) {
        case LogLevel::Info:
            tag = "[INFO]";
            break;
        case LogLevel::Warn:
            tag = "[WARNING]";
            break;
        case LogLevel::Debug:
            tag = "[DEBUG]";
            break;
        case LogLevel::Error:
            tag = "[ERROR]";
            break;
        case LogLevel::Fatal:
            tag = "[FATAL]";
            break;
        }
        os << "[CECommon]" << "[" << GetTimestamp() << "] " << tag << " " << GetLocation(loc) << message << std::endl;
    }
} // namespace CE