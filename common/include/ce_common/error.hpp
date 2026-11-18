#pragma once

#include <string>
#include <optional>
#include <system_error>

namespace CE {
    struct Error {
        std::string message;
        std::string context;

        std::error_code code; // gives us a numeric error
    };
}