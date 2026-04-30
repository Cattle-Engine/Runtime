#include <unistd.h>
#include <cstdlib>
#include <cstring>

#include "engine/platforms/linux.hpp"

namespace CE::Platforms::Linux {
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
}