#include "engine/instance.hpp"
#include "engine/bootstrap.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/vfs.hpp"

namespace CE {
    Instance::Instance(const char* data_file_name, bool debugmode) {
        CE::Log(CE::LogLevel::Info, "[Instance] Creating vfs instance");
        gVFS = new CE::VFS::VFS();
        CE::Log(CE::LogLevel::Info, "[Instance] Creating gameinfo instance");
        gGameInfo = new CE::GameInfo();

        CE::Log(CE::LogLevel::Info, "[Instance] Setting up game data");
        Bootstrap::Init_GameData(gVFS, gGameInfo, gDebug);

        Bootstrap::Init_GameInfo(gVFS, gGameInfo, gDebug);
    }

    Instance::~Instance() {

    }
}