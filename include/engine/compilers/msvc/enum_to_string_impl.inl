#pragma once

#include <string>
#include <string_view>

namespace CE::Utils {
    template<auto T>
    std::string EnumToString() {
        constexpr std::string_view function = __FUNCSIG__;
        constexpr std::string_view prefix = "EnumToString<";

        constexpr auto start = function.find(prefix) + prefix.size();
        constexpr auto end = function.find('>', start);

        std::string_view name = function.substr(start, end - start);

        // Faces::Back -> Back
        if (const auto scope = name.rfind("::");
            scope != std::string_view::npos) {
            name.remove_prefix(scope + 2);
        }

        return std::string(name);
    }
}