#include <string>

#include "engine/core.hpp"
#include "engine/gameinfo.hpp"
#include "engine/renderer.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/vfs.hpp"
#include "engine/common/vfs_stl.hpp"
#include "engine/common/error_box.hpp"
#include "engine/common/ini.hpp"

namespace CE::Bootstrap::GameSetup {
    void SetupGameData() {
        static CE::VFS::VFS g_vfs;
        CE::Global::SetVFS(&g_vfs);
        CE::Global::GetVFS().MountArchive(CE::GameInfo::dataFileName, "/", LoadMode::OnDemand);
        
        if (CE::Core::debugMode) {
            CE::Global::GetVFS().MountFolder("assets/", "/", LoadMode::OnDemand, 10);
        }
    }

    void SetupGameInfo() {
        auto stream = CE::VFS::OpenIStream(CE::Global::GetVFS(), "/Gameinfo.txt");
        
        if (!stream) {
            CE::Log(LogLevel::Fatal, "[Bootstrap] Unable to open Gameinfo.txt");
            ShowError("[Bootstrap] Gameinfo.txt is missing");
            std::exit(1);
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
            return;
        }

        if (!ini.has("Gameinfo", "Game_Name") || !ini.has("Gameinfo", "Game_Version") 
            ||  !ini.has("Graphics", "Window_Width") || !ini.has("Graphics", "Window_Height")
            || !ini.has("Graphics", "Max_FPS")) 
            {
                CE::Log(LogLevel::Fatal, "[Bootstrap] Missing required game info");
                ShowError("[Bootstrap] Missing required game info");
                std::exit(2);
            }

        CE::GameInfo::gameNameString = ini.get_string("Gameinfo", "Game_Name", "");
        CE::GameInfo::gameVersionString = ini.get_string("Gameinfo", "Game_Version", "");

        CE::GameInfo::windowWidth = ini.get_int("Graphics", "Window_Width", 0);
        CE::GameInfo::windowHeight = ini.get_int("Graphics", "Window_Height", 0);
        CE::GameInfo::windowTitle = ini.get_string("Graphics", "Window_Title", "");
        CE::GameInfo::maxFPS = ini.get_int("Graphics", "Max_FPS", 0);
        CE::Renderer::rendererName = ini.get_string("Graphics", "Renderer", "None");
        CE::GameInfo::enableVSync = ini.get_bool("Graphics", "Enable_VSync", false);

        CE::Log(LogLevel::Info, "[Bootstrap info] Game name: {}", CE::GameInfo::gameNameString);
        CE::Log(LogLevel::Info, "[Bootstrap Info] Game version: {}", CE::GameInfo::gameVersionString);
    }
}
