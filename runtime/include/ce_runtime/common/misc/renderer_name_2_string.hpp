#pragma once

#include <string>

#include "ce_runtime/common/misc/gameinfo.hpp"
#include "ce_runtime/rendering/renderer.hpp"

namespace CE::Common {
    void RendererName2String(const std::string renderername, RendererBackend& backend);
}