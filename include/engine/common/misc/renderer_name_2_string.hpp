#pragma once

#include <string>

#include "engine/common/misc/gameinfo.hpp"
#include "engine/renderer.hpp"

namespace CE::Common {
    void RendererName2String(const std::string renderername, RendererBackend& backend);
}