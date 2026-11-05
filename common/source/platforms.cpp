#include "ce_common/platforms.hpp"

#include <cstdlib>
#include <cstring>

#if defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#error "Unsupported platform"
#endif

namespace CECommon::Platforms {
#if defined(__linux__)
    bool SupportsANSI() {
        if (std::getenv("FORCE_COLOR")) {
            return true;
        }

        if (!isatty(STDOUT_FILENO)) {
            return false;
        }

        if (std::getenv("NO_COLOR")) {
            return false;
        }

        const char* term = std::getenv("TERM");
        if (!term || std::strcmp(term, "dumb") == 0) {
            return false;
        }

        return true;
    }

    bool EnableANSI() {
        return true;
    }
#endif

#if defined(_WIN32)
    bool SupportsANSI() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD mode = 0;
        if (!GetConsoleMode(hOut, &mode)) {
            return false;
        }

        return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    bool EnableANSI() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD mode = 0;
        if (!GetConsoleMode(hOut, &mode)) {
            return false;
        }

        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

        return SetConsoleMode(hOut, mode) != 0;
    }
#endif
} // namespace CECommon::Platforms
