#include <string>

#include "engine/bootstrap/instance.hpp"
#include "engine/common/fs/vfs_stl.hpp"
#include "engine/common/misc/gdat_has.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/fs/ini.hpp"
#include "engine/common/misc/error_box.hpp"

namespace CE::Bootstrap {
    int Init_GameData(std::unique_ptr<VFS::VFS>& vfs, const char* datafilename, bool debugmode) {
        CE_LOG(LogLevel::Info, "[Bootstrap] Game-data path name: {}", datafilename);
        vfs->MountArchive(datafilename, "/", LoadMode::OnDemand);
        
        /*if (debugmode) {
            vfs->MountFolder("assets/", "/", LoadMode::OnDemand, 10);
        }*/
        return 0;
    }

    int Init_GameInfo(std::unique_ptr<VFS::VFS>& vfs, std::unique_ptr<GameInfo>& gameinfo, bool debugmode) {
        auto stream = CE::VFS::OpenIStream(*vfs, "/Gameinfo.txt");
        
        if (!stream) {
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

        std::ostringstream ss;
        ss << stream->rdbuf();
        std::string text = ss.str();

        if (!CE::Ini::parse(text, ini, &err, opts)) {
            CE_LOG(LogLevel::Error, "[Bootstrap] Failed to parse Gameinfo.txt");
            ShowError("[Bootstrap] Failed to parse Gameinfo.txt");
            return 2;;
        }
        
        bool gresult = Common::GData_Has(text);

        if (!gresult) {
            CE_LOG(LogLevel::Error, "[Boostrap] Gameinfo.txt is missing required game-info");
            return 2;
        }

        gameinfo->gameNameString = ini.get_string("Gameinfo", "Game_Name", "");
        gameinfo->gameVersionString = ini.get_string("Gameinfo", "Game_Version", "");

        gameinfo->windowWidth = ini.get_int("Graphics", "Window_Width", 0);
        gameinfo->windowHeight = ini.get_int("Graphics", "Window_Height", 0);
        gameinfo->windowTitle = ini.get_string("Graphics", "Window_Title", "");
        gameinfo->maxFPS = ini.get_int("Graphics", "Max_FPS", 0);
        gameinfo->rendererName = ini.get_string("Graphics", "Renderer", "None");
        gameinfo->enableVSync = ini.get_bool("Graphics", "Enable_VSync", false);
        gameinfo->fullscreen = ini.get_bool("Graphics", "Fullscreen", false);
        gameinfo->resizableWindow = ini.get_bool("Graphics", "Resizable_Window");
        gameinfo->startupFileName = ini.get_string("Gameinfo", "Scripting_Startup_File", "startup.as");
        if(ini.has("Gameinfo", "Window_Icon")) {
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

        CE_LOG(LogLevel::Info, "[Bootstrap info] Game name: {}", gameinfo->gameNameString);
        CE_LOG(LogLevel::Info, "[Bootstrap Info] Game version: {}", gameinfo->gameVersionString);
        return 0;
    }
}