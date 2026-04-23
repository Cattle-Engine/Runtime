#pragma once

#include <string>

#include "engine/common/gameinfo.hpp"
#include "engine/renderer.hpp"

namespace CE::Common {
    void RendererName2String(const std::string renderername, RendererBackend& backend);
}