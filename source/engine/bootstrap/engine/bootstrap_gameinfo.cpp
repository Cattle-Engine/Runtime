#include <string>

#include "engine/bootstrap/engine.hpp"
#include "engine/gameinfo.hpp"
#include "engine/common/vfs.hpp"
#include "engine/common/vfs_stl.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/gdat_has.hpp"
#include "engine/common/ini.hpp"

namespace CE::Bootstrap::Engine {
    int GetGameInfo(GameInfo& gameinfo, std::string& gdata_name, bool debug) {
        VFS::VFS tmp_vfs;

        tmp_vfs.MountArchive(gdata_name.c_str(), "/", LoadMode::OnDemand);

        auto stream = VFS::OpenIStream(tmp_vfs, "Gameinfo.txt");

        if (!stream) {
            CE::Log(LogLevel::Error, "[Engine] Unable to open Gameinfo.txt :'(");
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

        bool gresult = Common::GData_Has(text);

        if (!gresult) {
            CE::Log(LogLevel::Error, "[Engine] Gameinfo.txt is missing required game-info");
            return 2;
        }

        gameinfo.gameNameString = ini.get_string("Gameinfo", "Game_Name", "");
        gameinfo.gameVersionString = ini.get_string("Gameinfo", "Game_Version", "");

        gameinfo.windowWidth = ini.get_int("Graphics", "Window_Width", 0);
        gameinfo.windowHeight = ini.get_int("Graphics", "Window_Height", 0);
        gameinfo.windowTitle = ini.get_string("Graphics", "Window_Title", "");
        gameinfo.maxFPS = ini.get_int("Graphics", "Max_FPS", 0);
        gameinfo.rendererName = ini.get_string("Graphics", "Renderer", "None");
        gameinfo.enableVSync = ini.get_bool("Graphics", "Enable_VSync", false);
        gameinfo.fullscreen = ini.get_bool("Graphics", "Fullscreen", false);
        gameinfo.resizableWindow = ini.get_bool("Graphics", "Resizable_Window");
    }
}