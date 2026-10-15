
#include "ce_common/detail/tracelog.hpp"
#include "ce_common/as_compiler/preprocessor.hpp"

#include <cctype>
#include <string>
#include <vector>

namespace CE::Scripting::Impl {
    namespace {
        struct Conditional {
            bool parent_active;
            bool branch_taken;
            bool active;
            bool seen_else = false;
        };

        std::string Trim(const std::string& str) {
            const auto first = str.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                return {};
            }

            const auto last = str.find_last_not_of(" \t\r\n");
            return str.substr(first, last - first + 1);
        }

        bool IsIdentifierStart(char c) {
            return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
        }

        bool IsIdentifierChar(char c) {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
        }

        bool IsIdentifier(const std::string& str) {
            if (str.empty() || !IsIdentifierStart(str[0])) {
                return false;
            }

            for (char c : str) {
                if (!IsIdentifierChar(c)) {
                    return false;
                }
            }

            return true;
        }

        bool IsDefined(
            const std::vector<Preprocessor::Define>& defines,
            const std::string& name
        ) {
            for (const auto& define : defines) {
                if (define.name == name) {
                    return true;
                }
            }

            return false;
        }

        std::string ReplaceDefines(
            const std::string& line,
            const std::vector<Preprocessor::Define>& defines,
            bool& in_block_comment
        ) {
            std::string result;
            result.reserve(line.size());

            bool in_string = false;
            bool in_char = false;
            bool escaped = false;

            for (std::size_t i = 0; i < line.size();) {
                const char c = line[i];
                const char next = i + 1 < line.size() ? line[i + 1] : '\0';

                if (in_block_comment) {
                    result += c;
                    ++i;

                    if (c == '*' && next == '/') {
                        result += '/';
                        ++i;
                        in_block_comment = false;
                    }

                    continue;
                }

                if (in_string || in_char) {
                    result += c;
                    ++i;

                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (in_string && c == '"') {
                        in_string = false;
                    } else if (in_char && c == '\'') {
                        in_char = false;
                    }

                    continue;
                }

                if (c == '/' && next == '/') {
                    result += line.substr(i);
                    break;
                }

                if (c == '/' && next == '*') {
                    result += "/*";
                    i += 2;
                    in_block_comment = true;
                    continue;
                }

                if (c == '"') {
                    result += c;
                    ++i;
                    in_string = true;
                    continue;
                }

                if (c == '\'') {
                    result += c;
                    ++i;
                    in_char = true;
                    continue;
                }

                if (!IsIdentifierStart(c)) {
                    result += c;
                    ++i;
                    continue;
                }

                const std::size_t start = i++;

                while (i < line.size() && IsIdentifierChar(line[i])) {
                    ++i;
                }

                const std::string name = line.substr(start, i - start);
                bool replaced = false;

                for (const auto& define : defines) {
                    if (define.name == name && !define.data.empty()) {
                        result += define.data;
                        replaced = true;
                        break;
                    }
                }

                if (!replaced) {
                    result += name;
                }
            }

            return result;
        }
    }

    bool Preprocessor::AddDefine(Define define_to_add) {
        if (!IsIdentifier(define_to_add.name)) {
            CE_LOG(
                CECommon::LogLevel::Error,
                "[ASPreprocessor] Invalid define name '{}'",
                define_to_add.name
            );
            return false;
        }

        for (const auto& define : mDefines) {
            if (define.name == define_to_add.name) {
                CE_LOG(
                    CECommon::LogLevel::Error,
                    "[ASPreprocessor] Define {} already exists",
                    define.name
                );
                return false;
            }
        }

        mDefines.push_back(define_to_add);
        return true;
    }

    bool Preprocessor::RemoveDefine(const std::string& name) {
        for (std::size_t i = 0; i < mDefines.size(); ++i) {
            if (name == mDefines[i].name) {
                mDefines.erase(mDefines.begin() + i);
                return true;
            }
        }

        CE_LOG(
            CECommon::LogLevel::Error,
            "[ASPreprocessor] Could not find define {}",
            name
        );
        return false;
    }

    bool Preprocessor::Process(
        const std::string& in,
        std::string& out
    ) {
        out.clear();

        std::vector<Conditional> conditions;
        bool in_block_comment = false;

        auto is_active = [&conditions]() {
            return conditions.empty() || conditions.back().active;
        };

        auto fail = [](const std::string& message) {
            CE_LOG(
                CECommon::LogLevel::Error,
                "[ASPreprocessor] {}",
                message
            );
            return false;
        };

        std::size_t position = 0;

        while (position < in.size()) {
            const std::size_t newline = in.find('\n', position);
            const bool has_newline = newline != std::string::npos;
            const std::size_t end = has_newline ? newline : in.size();

            const std::string line = in.substr(position, end - position);
            const std::string trimmed = Trim(line);

            position = has_newline ? end + 1 : end;

            // Directives are recognized at the beginning of a line.
            // A # inside a block comment is not a directive.
            if (!in_block_comment && !trimmed.empty() && trimmed[0] == '#') {
                std::size_t directive_end = 1;

                while (
                    directive_end < trimmed.size() &&
                    std::isspace(static_cast<unsigned char>(trimmed[directive_end]))
                ) {
                    ++directive_end;
                }

                std::size_t name_end = directive_end;

                while (
                    name_end < trimmed.size() &&
                    IsIdentifierChar(trimmed[name_end])
                ) {
                    ++name_end;
                }

                const std::string directive =
                    trimmed.substr(directive_end, name_end - directive_end);

                const std::string argument = Trim(trimmed.substr(name_end));

                if (directive == "define") {
                    if (is_active()) {
                        const std::size_t split = argument.find_first_of(" \t\r\n");

                        const std::string name =
                            split == std::string::npos
                                ? argument
                                : argument.substr(0, split);

                        const std::string data =
                            split == std::string::npos
                                ? ""
                                : Trim(argument.substr(split));

                        if (!IsIdentifier(name)) {
                            return fail("Invalid #define directive");
                        }

                        // Permit redefining an existing macro.
                        // This is convenient for engine configuration.
                        bool found = false;

                        for (auto& define : mDefines) {
                            if (define.name == name) {
                                define.data = data;
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            mDefines.push_back({name, data});
                        }
                    }
                } else if (directive == "ifdef" || directive == "ifndef") {
                    if (!IsIdentifier(argument)) {
                        return fail("Invalid #" + directive + " directive");
                    }

                    const bool parent = is_active();
                    const bool defined = IsDefined(mDefines, argument);
                    const bool matches =
                        directive == "ifdef" ? defined : !defined;
                    const bool active = parent && matches;

                    conditions.push_back({
                        parent,
                        active,
                        active,
                        false
                    });
                } else if (directive == "elif") {
                    if (conditions.empty()) {
                        return fail("#elif without matching #if");
                    }

                    if (!IsIdentifier(argument)) {
                        return fail("Invalid #elif directive");
                    }

                    Conditional& condition = conditions.back();

                    if (condition.seen_else) {
                        return fail("#elif after #else");
                    }

                    const bool matches = IsDefined(mDefines, argument);

                    condition.active =
                        condition.parent_active &&
                        !condition.branch_taken &&
                        matches;

                    condition.branch_taken =
                        condition.branch_taken || condition.active;
                } else if (directive == "else") {
                    if (!conditions.empty()) {
                        Conditional& condition = conditions.back();

                        if (!argument.empty() || condition.seen_else) {
                            return fail("Invalid or duplicate #else");
                        }

                        condition.active =
                            condition.parent_active &&
                            !condition.branch_taken;

                        condition.branch_taken =
                            condition.branch_taken || condition.active;

                        condition.seen_else = true;
                    } else {
                        return fail("#else without matching #if");
                    }
                } else if (directive == "endif") {
                    if (!argument.empty() || conditions.empty()) {
                        return fail("Invalid or unmatched #endif");
                    }

                    conditions.pop_back();
                } else {
                    return fail("Unknown directive #" + directive);
                }

                // Preserve line numbers in AngelScript diagnostics.
                out += '\n';
                continue;
            }

            if (is_active()) {
                out += ReplaceDefines(line, mDefines, in_block_comment);
            } else {
                // Still track block comments across inactive lines.
                ReplaceDefines(line, {}, in_block_comment);
            }

            out += '\n';
        }

        if (!conditions.empty()) {
            return fail("Unterminated conditional directive");
        }

        return true;
    }
}
