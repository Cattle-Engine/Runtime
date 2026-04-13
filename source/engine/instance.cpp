#include "engine/instance.hpp"
#include "engine/bootstrap.hpp"
#include "engine/common/tracelog.hpp"
#include <format>

namespace CE {
    Instance::Instance(const char* data_file_name, bool debugmode) {
        CE::Log(CE::LogLevel::Info, "[Instance] Setting up game data");
        int gds_return = Bootstrap::Init_GameData(gVFS.get(), gGameInfo.get(), gDebug);
        if(gds_return != 0) {
            throw std::runtime_error(
                std::format("[Instance] Gamedata mount returned with code {}", gds_return));
        }

        CE::Log(CE::LogLevel::Info, "[Instance] Creating game info");
        int gis_return = Bootstrap::Init_GameInfo(gVFS.get(), gGameInfo.get(), gDebug);
        if(gis_return != 0) {
            throw std::runtime_error(
                std::format("[Instance] Failed to get gameinfo with code: {}", gis_return));
        }


        CE::Log(CE::LogLevel::Info, "[Instance] Creating window & renderer");
        int vis = CE::Bootstrap::Init_Video(gGameInfo.get(), gDebug, gRenderer, gRendererBackend, gWindow);
        if(vis != 0) {
            throw std::runtime_error(
                std::format("[Instance] Video setup returned with: {}", vis));
        }
    }

    Instance::~Instance() {
    }
}