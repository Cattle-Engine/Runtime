#include <SDL3/SDL.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_messagebox.h>

#include <ce_runtime_api.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

std::string LoadPath(const std::filesystem::path& base_path) {
    std::ifstream file(base_path / "path.cfg");

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open path.cfg");
    }

    std::string path;

    if (!std::getline(file, path)) {
        throw std::runtime_error("path.cfg is empty or has no first line");
    }

    // Handle Windows CRLF line endings.
    if (!path.empty() && path.back() == '\r') {
        path.pop_back();
    }

    if (path.empty()) {
        throw std::runtime_error("The first line of path.cfg is empty");
    }

    return path;
}

int main(int argc, char** argv) {
    ce_engine_arguments_t engine_args{};
    CE_ParseProgramArguments(argc, argv, &engine_args);

    const char* base_path = SDL_GetBasePath();

    if (base_path == nullptr) {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "CE Loader error",
            "Failed to get executable base path",
            nullptr
        );
        return 1;
    }

    ce_engine_t* engine = nullptr;

    try {
        const std::string data_file_path =
            LoadPath(std::filesystem::u8path(base_path));

        engine = CE_CreateEngine(data_file_path.c_str(), engine_args, true);

        if (engine == nullptr) {
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                "CE Loader error",
                "Failed to create engine",
                nullptr
            );
            return 3;
        }

        if (!CE_EngineCreateInstance(engine, "main", true, nullptr)) {
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                "CE Loader error",
                "Failed to create instance",
                nullptr
            );

            CE_DestroyEngine(engine);
            return 4;
        }

        const int result = CE_EngineRun(engine);
        CE_DestroyEngine(engine);
        return result;

    } catch (const std::exception& e) {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "CE Loader error",
            e.what(),
            nullptr
        );

        if (engine != nullptr) {
            CE_DestroyEngine(engine);
        }

        return 2;
    }
}