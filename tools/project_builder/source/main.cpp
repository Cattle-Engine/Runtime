#include <cstdint>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>

#include "yaml-cpp/yaml.h"

#include "gameinfo.hpp"

constexpr std::string_view kVersionText = "0.1.0";

namespace FS = std::filesystem;
enum class CEGrillerReturns {
    ArgumentParseFail,
    CreateNewProjectFail,
    BuildProjectFail,
    Okay,
};

template <typename T>
constexpr std::uint32_t ToUint32(T value) {
    return static_cast<std::uint32_t>(value);
}

struct CEBuildInfo {
    uint32_t manifest_version = 0;

    std::vector<FS::path> asset_paths;
    std::vector<std::string> exclude_files;

    FS::path build_output;
    std::string game_data_file_name;
    CE::GameInfo gameinfo;
};


struct Options {
    bool build_project = false;
    bool create_new_project = false;
    bool show_help = false;
    bool show_version = false;

    FS::path project_root;
};

void ShowHelp() {
    std::cout << "Usage: ce_griller [COMMAND] [PROJECT_PATH]\n"
              << "       ce_griller [COMMAND] [OPTIONS] [PROJECT_PATH]\n"
              << "Commands:\n"
              << "  build           Builds a CE project\n"
              << "  new             Creates a new CE project\n"
              << "Options:\n"
              << "  -h, --help      Shows this help message\n"
              << "  -v, --version   Shows the version\n";
}

void PrintVersion() {
    std::cout << "ce_griller " << kVersionText << "\n";
}

bool ParseArguments(int argc, char* argv[], Options& options) {
    std::vector<std::string_view> positional;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);

        if (arg == "build") {
            options.build_project = true;
            continue;
        }

        if (arg == "new") {
            options.create_new_project = true;
            continue;
        }

        if (arg == "-h" || arg == "--help") {
            options.show_help = true;
            continue;
        }

        if (arg == "-v" && arg == "--version") {
            options.show_version = true;
            continue;
        }

        if (!arg.empty() && arg.front() == '-') {
            std::cerr << "ce_griller: unknown option '" << arg << "'\n";
            return false;
        }

        positional.emplace_back(arg);
    }

    if (options.show_help) {
        return true;
    }

    if (options.build_project == options.create_new_project) {
        std::cerr << "ce_griller: expected a command ('build' or 'new')\n";
        return false;
    }

    if (positional.size() != 1) {
        std::cerr << "ce_griller: expected exactly one project directory\n";
        return false;
    }

    options.project_root = positional.front();

    return true;
}

bool CreateNewProject(const Options& options) {
    return false;
}

bool BuildProject(const Options& options) {
    return false;
}

int main(int argc, char* argv[]) {
    Options options;
    if (!ParseArguments(argc, argv, options)) {
        ShowHelp();
        return ToUint32<CEGrillerReturns>(CEGrillerReturns::ArgumentParseFail);
    }

    if (options.show_help) {
        ShowHelp();
        return ToUint32<CEGrillerReturns>(CEGrillerReturns::Okay);
    }

    if (options.show_version) {
        PrintVersion();
        return ToUint32<CEGrillerReturns>(CEGrillerReturns::Okay);
    }

    if (options.create_new_project) {
        if (!CreateNewProject(options)) {
            return ToUint32<CEGrillerReturns>(CEGrillerReturns::CreateNewProjectFail);
        } 
        return ToUint32<CEGrillerReturns>(CEGrillerReturns::Okay);
    }

    if (options.build_project) {
        if (!BuildProject(options)) {
            return ToUint32<CEGrillerReturns>(CEGrillerReturns::BuildProjectFail);
        } 
        return ToUint32<CEGrillerReturns>(CEGrillerReturns::Okay);
    }

    return 0;
}