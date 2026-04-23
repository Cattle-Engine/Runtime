#include "engine/common/gameinfo.hpp"
#include "engine/renderer.hpp"
#include "engine/common/renderer_name_2_string.hpp"

namespace CE::Common {
    void RendererName2String(const std::string renderername, RendererBackend& backend) {
        if (renderername == "None") {
            backend = RendererBackend::None;
        } else if (renderername == "Software") {
            backend = RendererBackend::Software;
        } else if (renderername == "OpenGL") {
            backend = RendererBackend::OpenGL;
        } else if (renderername == "DX12") {
            backend = RendererBackend::DX12;
        } else if (renderername == "Metal") {
            backend = RendererBackend::Metal;
        } else if (renderername == "Vulkan") {
            backend = RendererBackend::Vulkan;
        } else {
            backend = RendererBackend::None;
        }
        return;
    }
}