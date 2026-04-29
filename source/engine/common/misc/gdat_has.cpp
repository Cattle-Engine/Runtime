#include <string>

#include "engine/common/tracelog.hpp"
#include "engine/common/fs/ini.hpp"

namespace CE::Common {
    bool GData_Has(std::string text) {  
        CE::Ini::IniFile ini;
        CE::Ini::ParseError err;
        CE::Ini::Options opts;
        opts.allow_inline_comments = true;
        opts.allow_colon_delim = true;
        opts.allow_empty_values = false;

        if (!CE::Ini::parse(text, ini, &err, opts)) {
            CE::Log(LogLevel::Error, "[Common] [Gameinfo Parser] Failed to parse Gameinfo.txt");
            return false;
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
                return false;
            }
        return true;
    }
}