#include <string>

#include "engine/bootstrap/instance.hpp"
#include "engine/common/vfs_stl.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/ini.hpp"
#include "engine/common/error_box.hpp"

namespace CE::Bootstrap {
    int Init_GameData(std::unique_ptr<VFS::VFS>& vfs, const char* datafilename, bool debugmode) {
        CE::Log(LogLevel::Info, "[Bootstrap] Game-data path name: {}", datafilename);
        vfs->MountArchive(datafilename, "/", LoadMode::OnDemand);
        
        /*if (debugmode) {
            vfs->MountFolder("assets/", "/", LoadMode::OnDemand, 10);
        }*/
        return 0;
    }

    int Init_GameInfo(std::unique_ptr<VFS::VFS>& vfs, std::unique_ptr<GameInfo>& gameinfo, bool debugmode) {
        auto stream = CE::VFS::OpenIStream(*vfs, "/Gameinfo.txt");
        
        if (!stream) {
            CE::Log(LogLevel::Fatal, "[Bootstrap] Unable to open Gameinfo.txt");
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
            CE::Log(LogLevel::Error, "[Bootstrap] Failed to parse Gameinfo.txt");
            ShowError("[Bootstrap] Failed to parse Gameinfo.txt");
            return 2;;
        }
        
        if (!ini.has("Gameinfo", "Game_Name") ||
            !ini.has("Gameinfo", "Game_Version") ||
            !ini.has("Graphics", "Window_Width") ||
            !ini.has("Graphics", "Window_Height") ||
            !ini.has("Graphics", "Window_Title") ||
            !ini.has("Graphics", "Max_FPS") ||
            !ini.has("Graphics", "Renderer") ||
            !ini.has("Graphics", "Enable_VSync") ||
            !ini.has("Graphics", "Fullscreen") ||
            !ini.has("Graphics", "Resizable_Window"))
            {
                CE::Log(LogLevel::Fatal, "[Bootstrap] Missing required game info");
                ShowError("[Bootstrap] Missing required game info");
                return 3;;
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

        CE::Log(LogLevel::Info, "[Bootstrap info] Game name: {}", gameinfo->gameNameString);
        CE::Log(LogLevel::Info, "[Bootstrap Info] Game ve>rsion: {}", gameinfo->gameVersionString);
        return 0;
    }
}