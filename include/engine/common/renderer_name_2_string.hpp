#pragma once

#include "engine/gameinfo.hpp"
#include "engine/renderer.hpp"

namespace CE::Common {
    void RendererName2String(const GameInfo& gameinfo, RendererBackend& backend);
}