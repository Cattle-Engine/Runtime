#include <angelscript.h>

#include "engine/common/tracelog.hpp"

#include <format>
#include <source_location>
#include <stdexcept>
#include <string>

namespace CE {
    static std::string FormatScriptArgument(
        asIScriptGeneric* gen,
        asUINT index
    ) {
        const int type_Id = gen->GetArgTypeId(index);
        const int string_type_id = gen->GetEngine()->GetTypeIdByDecl("string");

        if (type_Id == string_type_id) {
            auto* value =
                static_cast<std::string*>(gen->GetArgAddress(index));

            return *value;
        }

        switch (type_Id) {
            case asTYPEID_BOOL:
                return std::format("{}", gen->GetArgByte(index) != 0);

            case asTYPEID_INT8:
                return std::format(
                    "{}",
                    static_cast<int8_t>(gen->GetArgByte(index))
                );

            case asTYPEID_INT16:
                return std::format(
                    "{}",
                    static_cast<int16_t>(gen->GetArgWord(index))
                );

            case asTYPEID_INT32:
                return std::format(
                    "{}",
                    static_cast<int32_t>(gen->GetArgDWord(index))
                );

            case asTYPEID_INT64:
                return std::format(
                    "{}",
                    static_cast<int64_t>(gen->GetArgQWord(index))
                );

            case asTYPEID_UINT8:
                return std::format(
                    "{}",
                    static_cast<uint8_t>(gen->GetArgByte(index))
                );

            case asTYPEID_UINT16:
                return std::format(
                    "{}",
                    static_cast<uint16_t>(gen->GetArgWord(index))
                );

            case asTYPEID_UINT32:
                return std::format(
                    "{}",
                    static_cast<uint32_t>(gen->GetArgDWord(index))
                );

            case asTYPEID_UINT64:
                return std::format(
                    "{}",
                    static_cast<uint64_t>(gen->GetArgQWord(index))
                );

            case asTYPEID_FLOAT:
                return std::format("{}", gen->GetArgFloat(index));

            case asTYPEID_DOUBLE:
                return std::format("{}", gen->GetArgDouble(index));

            default:
                break;
        }

        asIScriptContext* ctx = asGetActiveContext();

        if (ctx) {
            ctx->SetException("Not enough arguments for format string");
        }

        return "";
    }

    void ScriptLog(asIScriptGeneric* gen) {
        auto level = static_cast<CE::LogLevel>(gen->GetArgDWord(0));

        auto* format =
            static_cast<std::string*>(gen->GetArgAddress(1));

        std::string result;
        result.reserve(format->size());

        asUINT argument = 2;

        for (size_t i = 0; i < format->size(); ++i) {
            if ((*format)[i] == '{' &&
                i + 1 < format->size() &&
                (*format)[i + 1] == '}') {

                if (argument >= static_cast<asUINT>(gen->GetArgCount())) {
                    throw std::runtime_error(
                        "Not enough arguments for format string"
                    );
                }

                result += FormatScriptArgument(gen, argument);

                ++argument;
                ++i;
                continue;
            }

            result += (*format)[i];
        }

        if (argument != static_cast<asUINT>(gen->GetArgCount())) {
            throw std::runtime_error(
                "Too many arguments for format string"
            );
        }

        CE::LogImpl(level, result, std::source_location::current());
    }
}