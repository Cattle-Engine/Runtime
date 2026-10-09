#pragma once

#include <string>
#include <string_view>

namespace CE::Utils {
    template<auto T>
    std::string EnumToString() {
        constexpr std::string_view function = __PRETTY_FUNCTION__;
        constexpr std::string_view prefix = "T = ";

        constexpr auto start = function.find(prefix) + prefix.size();
        constexpr auto end = function.find(']', start);

        std::string_view name = function.substr(start, end - start);

        // GCC may have extra template information after the value.
        if (const auto semicolon = name.find(';');
            semicolon != std::string_view::npos) {
            name = name.substr(0, semicolon);
        }

        // Faces::Back -> Back
        if (const auto scope = name.rfind("::");
            scope != std::string_view::npos) {
            name.remove_prefix(scope + 2);
        }

        return std::string(name);
    }
}