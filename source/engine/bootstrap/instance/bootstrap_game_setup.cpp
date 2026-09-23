#include <string>

#include "engine/bootstrap/instance.hpp"
#include "engine/common/fs/ini.hpp"
#include "engine/common/fs/os_fs/directory_file_provider.hpp"
#include "engine/common/fs/tcf/tcf_file_provider.hpp"
#include "engine/common/misc/error_box.hpp"
#include "engine/common/misc/gdat_has.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/window.hpp"

namespace CE::Bootstrap {
    int Init_GameData(std::unique_ptr<Common::FS::VFS::VFS>& vfs, const char* datafilename, bool debugmode) {
        CE_LOG(LogLevel::Info, "[Bootstrap] Game-data path name: {}", datafilename);
        vfs->AddMountPoint<Common::FS::TCF::TCFFileProvider>("/", 0, datafilename);

        if (debugmode) {
            vfs->AddMountPoint<Common::FS::OSFS::DirectoryFileProvider>("/", 10, "assets/");
        }
        return 0;
    }

    int Init_GameInfo(std::unique_ptr<Common::FS::VFS::VFS>& vfs, std::unique_ptr<GameInfo>& gameinfo,
                      [[maybe_unused]] bool debugmode) {
        auto file = vfs->OpenFile("/Gameinfo.txt");
        if (!file) {
            CE_LOG(LogLevel::Fatal, "[Bootstrap] Unable to open Gameinfo.txt");
            ShowError("[Bootstrap] Gameinfo.txt is missing");
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
            CE_LOG(LogLevel::Fatal, "[Bootstrap] Unable to read Gameinfo.txt");
            return 1;
        }

        if (!CE::Ini::parse(text, ini, &err, opts)) {
            CE_LOG(LogLevel::Fatal, "[Bootstrap] Failed to parse Gameinfo.txt");
            ShowError("[Bootstrap] Failed to parse Gameinfo.txt");
            return 2;
            ;
        }

        bool gresult = Common::GData_Has(text);

        if (!gresult) {
            CE_LOG(LogLevel::Fatal, "[Boostrap] Gameinfo.txt is missing required game-info");
            return 2;
        }

        int window_mode = 0;

        gameinfo->gameNameString = ini.get_string("Gameinfo", "Game_Name", "");
        gameinfo->gameVersionString = ini.get_string("Gameinfo", "Game_Version", "");

        gameinfo->windowWidth = ini.get_int("Graphics", "Window_Width", 0);
        gameinfo->windowHeight = ini.get_int("Graphics", "Window_Height", 0);
        gameinfo->windowTitle = ini.get_string("Graphics", "Window_Title", "");
        gameinfo->maxFPS = ini.get_int("Graphics", "Max_FPS", 0);
        gameinfo->rendererName = ini.get_string("Graphics", "Renderer", "None");
        gameinfo->enableVSync = ini.get_bool("Graphics", "Enable_VSync", false);
        window_mode = ini.get_int("Graphics", "Window_Mode", 0);
        gameinfo->resizableWindow = ini.get_bool("Graphics", "Resizable_Window");
        gameinfo->startupFileName = ini.get_string("Gameinfo", "Scripting_Startup_File", "startup.as");
        if (ini.has("Gameinfo", "Window_Icon")) {
            gameinfo->windowIcon = ini.get_string("Gameinfo", "Window_Icon", "");
        }

        gameinfo->pauseRenderingWhenFocusLostInWindowedMode = ini.get_bool("Graphics", "No_Render_On_Focus_Lost", true);
        gameinfo->pauseUpdateWhenFocusLost = ini.get_bool("Graphics", "No_Update_On_Focus_Lost", true);

        gameinfo->maxWindowWidth = ini.get_int("Graphics", "Max_Window_Width", 640);
        gameinfo->maxWindowHeight = ini.get_int("Graphics", "Max_Window_Height", 360);
        gameinfo->minWindowWidth = ini.get_int("Graphics", "Min_Window_Width", 3840);
        gameinfo->minWindowHeight = ini.get_int("Graphics", "Min_Window_Height", 2160);

        if (gameinfo->minWindowWidth > gameinfo->maxWindowWidth) {
            std::swap(gameinfo->minWindowWidth, gameinfo->maxWindowWidth);
        }

        if (gameinfo->minWindowHeight > gameinfo->maxWindowHeight) {
            std::swap(gameinfo->minWindowHeight, gameinfo->maxWindowHeight);
        }

        if (window_mode >= 0 && window_mode <= 2) {
            gameinfo->windowMode = static_cast<Common::Window::WindowMode>(window_mode);
        } else {
            CE_LOG(LogLevel::Error, "[Bootstrap] Window_Mode is out of bounds, using default (windowed)");
            CE_LOG(LogLevel::Debug, "0 = Fullscreen, 1 = Borderless, 2 = Windowed");
            gameinfo->windowMode = Common::Window::WindowMode::Windowed;
        }

        CE_LOG(LogLevel::Info, "[Bootstrap] Game name: {}", gameinfo->gameNameString);
        CE_LOG(LogLevel::Info, "[Bootstrap] Base game version: {}", gameinfo->gameVersionString);
        return 0;
    }
} // namespace CE::Bootstrap
