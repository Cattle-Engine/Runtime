#include "engine/renderer.hpp"
#include "engine/renderers/vulkan.hpp"

namespace CE::Renderer {
    CE::Renderer::IRenderer* CreateRenderer(RendererBackend backend) {
        switch (backend) {
            case RendererBackend::Vulkan:
                return new CE::Renderer::Vulkan::VulkanRenderer();
            default:
                return nullptr;
        }
    }
}