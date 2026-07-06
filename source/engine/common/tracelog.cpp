#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <ostream>

#include "engine/platforms.hpp"
#include "engine/common/tracelog.hpp"

namespace {
    bool kIsDebug =
    #if defined(CE_DEBUG)
        true;
    #else
        false;
    #endif
    using Manip = std::ostream& (*)(std::ostream&);
}

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

        Manip colour = nullptr;
        std::string tag;

        switch (level) {
            case Info:
                colour = blue;
                tag = "[INFO]";
                break;
            case Warn:
                colour = yellow;
                tag = "[WARNING]";
                break;
            case Debug:
                colour = blue;
                tag = "[DEBUG]";
                break;
            case Error:
                colour = red;
                tag = "[ERROR]";
                break;
            case Fatal:
                colour = bold_red;
                tag = "[FATAL]";
                break;
        }

        if (colour)
            os << colour;

        os << "[" << GetTimestamp() << "] "
        << tag << " "
        << GetLocation(loc)
        << message
        << std::endl;
    }
}