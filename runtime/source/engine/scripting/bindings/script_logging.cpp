#include <angelscript.h>

#include "engine/common/tracelog.hpp"

#include "engine/scripting/bindings/script_logging.hpp"

#include <format>
#include <stdexcept>
#include <string>
#include <utility>

namespace CE::Scripting::Bindings {
    static std::string FormatScriptArgument(
        asIScriptGeneric* gen,
        asUINT index
    ) {
        const int type_id = gen->GetArgTypeId(index);
        const int string_type_id = gen->GetEngine()->GetTypeIdByDecl("string");

        if (type_id == string_type_id) {
            auto* value =
                static_cast<std::string*>(gen->GetArgAddress(index));

            return *value;
        }

        switch (type_id) {
            case asTYPEID_BOOL:
                return std::format("{}", *static_cast<bool*>(gen->GetArgAddress(index)));

            case asTYPEID_INT8:
                return std::format("{}", *static_cast<int8_t*>(gen->GetArgAddress(index)));

            case asTYPEID_INT16:
                return std::format("{}", *static_cast<int16_t*>(gen->GetArgAddress(index)));

            case asTYPEID_INT32:
                return std::format("{}", *static_cast<int32_t*>(gen->GetArgAddress(index)));

            case asTYPEID_INT64:
                return std::format("{}", *static_cast<int64_t*>(gen->GetArgAddress(index)));

            case asTYPEID_UINT8:
                return std::format("{}", *static_cast<uint8_t*>(gen->GetArgAddress(index)));

            case asTYPEID_UINT16:
                return std::format("{}", *static_cast<uint16_t*>(gen->GetArgAddress(index)));

            case asTYPEID_UINT32:
                return std::format("{}", *static_cast<uint32_t*>(gen->GetArgAddress(index)));

            case asTYPEID_UINT64:
                return std::format("{}", *static_cast<uint64_t*>(gen->GetArgAddress(index)));

            case asTYPEID_FLOAT:
                return std::format("{}", *static_cast<float*>(gen->GetArgAddress(index)));

            case asTYPEID_DOUBLE:
                return std::format("{}", *static_cast<double*>(gen->GetArgAddress(index)));

            default:
                break;
        }

        asIScriptContext* ctx = asGetActiveContext();

        if (ctx) {
            ctx->SetException("Not enough arguments for format string", false);
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

        if (std::cmp_not_equal(argument ,gen->GetArgCount())) {
            throw std::runtime_error(
                "Too many arguments for format string"
            );
        }

        asIScriptContext* ctx = asGetActiveContext();

        int line = 0;
        int column = 0;
        const char* section = nullptr;
        if (ctx) {
            line = ctx->GetLineNumber(0, &column, &section);
        }

        CE::Log(
            level,
            "[Angelscript] " + result,
            section ? section : "<unknown>", 
            line
        );
    }
}