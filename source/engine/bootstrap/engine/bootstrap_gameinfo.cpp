#include <string>

#include "engine/bootstrap/engine.hpp"
#include "engine/gameinfo.hpp"
#include "engine/common/vfs.hpp"
#include "engine/common/vfs_stl.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/error_box.hpp"
#include "engine/common/ini.hpp"

namespace CE::Bootstrap::Engine {
    int GetGameInfo(GameInfo& gameinfo, std::string& gdata_name, bool debug) {
        VFS::VFS tmp_vfs;

        tmp_vfs.MountArchive(gdata_name.c_str(), "/", LoadMode::OnDemand);

        auto stream = VFS::OpenIStream(tmp_vfs, "Gameinfo.txt");

        if (!stream) {
            CE::Log(LogLevel::Error, "[Engine] Unable to open Gameinfo.txt :'(");
            ShowError("[Engine] Unable to open Gameinfo.txt");
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

            
    }
}