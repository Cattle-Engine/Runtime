#include "engine/engine.hpp"
#include "engine/common/misc/arguments.hpp"
#include "public/ce_runtime_api.h"

#include <memory>
#include <new>
#include <optional>
#include <string>
#include <vector>

// The implementation is hidden from C consumers.
struct ce_engine_t {
    std::unique_ptr<CE::Engine> impl;
};

// Stores the parsed path returned by CE_ParseProgramArguments.
// Its pointer remains valid until the next call on the same thread.
static thread_local std::string gParsedDataFilePath;

extern "C" {

CE_EXPORT void CE_ParseProgramArguments(
    int argc,
    const char* const* argv,
    ce_engine_arguments_t* output
) {
    if (output == nullptr || argc < 0 || (argc > 0 && argv == nullptr)) {
        return;
    }

    CE::EngineArguements parsed{};
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));

    for (int i = 0; i < argc; ++i) {
        if (argv[i] == nullptr) {
            return;
        }

        args.emplace_back(argv[i]);
    }

    try {
        CE::Common::ParseProgramArguments(args, parsed);

        gParsedDataFilePath = parsed.OutputDebugASInfoPath;

        output->output_debug_as_info = parsed.OutputDebugASInfo;
        output->output_debug_as_info_path = gParsedDataFilePath.c_str();
        output->debug_video = parsed.DebugVideo;
    } catch (...) {
        output->output_debug_as_info = false;
        output->output_debug_as_info_path = nullptr;
        output->debug_video = false;
    }
}

CE_EXPORT ce_engine_t* CE_CreateEngine(const char* data_file, ce_engine_arguments_t args, bool debug_video) {
    try {
        CE::EngineArguements engine_args{};
        engine_args.OutputDebugASInfo = args.output_debug_as_info;
        engine_args.OutputDebugASInfoPath = args.output_debug_as_info_path
            ? args.output_debug_as_info_path
            : "";

        auto engine = std::make_unique<ce_engine_t>();
        engine->impl = std::make_unique<CE::Engine>(
            data_file ? data_file : "",
            debug_video,
            engine_args
        );

        return engine.release();
    } catch (...) {
        return nullptr;
    }
}

CE_EXPORT bool CE_DestroyEngine(ce_engine_t* engine) {
    if (engine == nullptr) {
        return false;
    }

    delete engine;
    return true;
}

CE_EXPORT bool CE_EngineCreateInstance(
    ce_engine_t* engine,
    const char* name,
    bool debug,
    const char* data_file
) {
    if (engine == nullptr || engine->impl == nullptr || name == nullptr) {
        return false;
    }

    try {
        std::optional<std::string> filename;

        if (data_file != nullptr) {
            filename = data_file;
        }

        return engine->impl->CreateInstance(
            std::string(name),
            debug,
            filename
        );
    } catch (...) {
        return false;
    }
}

CE_EXPORT bool CE_EngineDestroyInstance(
    ce_engine_t* engine,
    const char* name
) {
    if (engine == nullptr || engine->impl == nullptr || name == nullptr) {
        return false;
    }

    try {
        return engine->impl->DestroyInstance(std::string(name));
    } catch (...) {
        return false;
    }
}

CE_EXPORT int CE_EngineUpdateInstance(
    ce_engine_t* engine,
    const char* name
) {
    if (engine == nullptr || engine->impl == nullptr || name == nullptr) {
        return -1;
    }

    try {
        return engine->impl->UpdateInstance(std::string(name));
    } catch (...) {
        return -1;
    }
}

CE_EXPORT int CE_EngineRun(ce_engine_t* engine) {
    if (engine == nullptr || engine->impl == nullptr) {
        return -1;
    }

    try {
        return engine->impl->Run();
    } catch (...) {
        return -1;
    }
}

}