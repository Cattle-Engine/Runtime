#include "engine/gameinfo.hpp"
#include "engine/renderer.hpp"

namespace CE::Common {
    void RendererName2String(const GameInfo& gameinfo, RendererBackend& backend) {
        if (gameinfo.rendererName == "None") {
            backend = RendererBackend::None;
        } else if (gameinfo.rendererName == "Software") {
            backend = RendererBackend::Software;
        } else if (gameinfo.rendererName == "OpenGL") {
            backend = RendererBackend::OpenGL;
        } else if (gameinfo.rendererName == "DX12") {
            backend = RendererBackend::DX12;
        } else if (gameinfo.rendererName == "Metal") {
            backend = RendererBackend::Metal;
        } else if (gameinfo.rendererName == "Vulkan") {
            backend = RendererBackend::Vulkan;
        } else {
            backend = RendererBackend::None;
        }
        return;
    }
}