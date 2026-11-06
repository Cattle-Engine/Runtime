#include <string>
#include <algorithm>

#include "engine/bootstrap/engine.hpp"
#include "engine/common/fs/ini.hpp"
#include "engine/common/fs/vfs.hpp"
#include "engine/common/fs/tcf/tcf_file_provider.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/common/misc/gdat_has.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Bootstrap::Engine {
    int GetGameInfo(GameInfo& gameinfo, std::string& gdata_name, [[maybe_unused]] bool debug) {
        Common::FS::VFS::VFS tmp_vfs;

        tmp_vfs.AddMountPoint<Common::FS::TCF::TCFFileProvider>("/", 0, gdata_name);

        const bool exists = tmp_vfs.FileExists("Gameinfo.txt");

        CE_LOG(
            LogLevel::Debug,
            "[Engine] VFS FileExists(Gameinfo.txt) = {}",
            exists
        );

        const auto provider = tmp_vfs.GetProviderNameOfMountPath("Gameinfo.txt");

        CE_LOG(
            LogLevel::Debug,
            "[Engine] Provider = '{}'",
            provider
        );

        auto file = tmp_vfs.OpenFile("Gameinfo.txt");

        if (!file) {
            CE_LOG(LogLevel::Error, "[Engine] Unable to open Gameinfo.txt :'(");
            return 1;
        }

        CE::Ini::IniFile ini;
        CE::Ini::ParseError err;
        CE::Ini::Options opts;
        opts.allow_inline_comments = true;
        opts.allow_colon_delim = true;
        opts.allow_empty_values = false;

        std::string text(file->Size(), '\0');
        if (!text.empty() && !file->Read(text.data(), text.size())) {
            CE_LOG(LogLevel::Error, "[Engine] Unable to read Gameinfo.txt");
            return 1;
        }

        bool gresult = Common::GData_Has(text);

        if (!gresult) {
            CE_LOG(LogLevel::Error, "[Engine] Gameinfo.txt is missing required game-info");
            return 2;
        }

        if (!CE::Ini::parse(text, ini, &err, opts)) {
            CE_LOG(LogLevel::Error, "[Common] [Gameinfo Parser] Failed to parse Gameinfo.txt");
            return 3;
        }

        gameinfo.game_name = ini.get_string("Gameinfo", "Game_Name", "");
        gameinfo.game_version = ini.get_string("Gameinfo", "Game_Version", "");

        gameinfo.windowWidth = ini.get_int("Graphics", "Window_Width", 0);
        gameinfo.windowHeight = ini.get_int("Graphics", "Window_Height", 0);
        gameinfo.windowTitle = ini.get_string("Graphics", "Window_Title", "");
        gameinfo.maxFPS = ini.get_int("Graphics", "Max_FPS", 0);
        gameinfo.rendererName = ini.get_string("Graphics", "Renderer", "None");
        gameinfo.enableVSync = ini.get_bool("Graphics", "Enable_VSync", false);
        int window_mode = ini.get_int("Graphics", "Window_Mode", 0);
        gameinfo.resizableWindow = ini.get_bool("Graphics", "Resizable_Window");
        
        gameinfo.game_package_id = ini.get_string("Gameinfo", "Game_PackageID");
        
        const std::string& package_id = gameinfo.game_package_id;
        
        if (package_id.empty() ||
            package_id.front() == '.' ||
            package_id.back() == '.' ||
            package_id.find("..") != std::string::npos ||
            !std::all_of(package_id.begin(), package_id.end(), [](unsigned char c) {
                    return (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') ||
                    c == '.' || c == '_' || c == '-';
                })) {
                CE_LOG(LogLevel::Error, "[Bootstrap] Invalid Game_PackageID '{}'", package_id);
            return 4;
        }
        
        if (gameinfo.minWindowWidth > gameinfo.maxWindowWidth) {
            std::swap(gameinfo.minWindowWidth, gameinfo.maxWindowWidth);
        }

        if (gameinfo.minWindowHeight > gameinfo.maxWindowHeight) {
            std::swap(gameinfo.minWindowHeight, gameinfo.maxWindowHeight);
        }

        if (window_mode >= 0 && window_mode <= 2) {
            gameinfo.windowMode = static_cast<Common::WindowMode>(window_mode);
        } else {
            CE_LOG(LogLevel::Error, "[Bootstrap] Window_Mode is out of bounds, using default (windowed)");
            CE_LOG(LogLevel::Debug, "0 = Fullscreen, 1 = Borderless, 2 = Windowed");
            gameinfo.windowMode = Common::WindowMode::Windowed;
        }
        return 0;
    }
} // namespace CE::Bootstrap::Engine
