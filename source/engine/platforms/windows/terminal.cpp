#include <windows.h>
#include "engine/platforms/windows.hpp"

namespace CE::Platforms::Windows {
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
}