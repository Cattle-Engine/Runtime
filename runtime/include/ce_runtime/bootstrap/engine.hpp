#pragma once

#include <string>

#include "ce_runtime/common/misc/gameinfo.hpp"

namespace CE::Bootstrap::Engine {
    int GetGameInfo(GameInfo& gameinfo, std::string& gdata_name, bool debug);
}