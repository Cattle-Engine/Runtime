#pragma once

#include <string>
#include "engine/common/gameinfo.hpp"fo.hpp"

namespace CE::Bootstrap::Engine {
    int GetGameInfo(GameInfo& gameinfo, std::string& gdata_name, bool debug);
}