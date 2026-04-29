#include <memory>

#include "engine/renderers/software_renderer.hpp"

namespace CE::Renderer::Software {
    GPUDeviceHandle CreateGPUDevice() {
        auto device = std::make_shared<Renderer::GPUDevice>();
        device->device = nullptr;
        device->backend = RendererBackend::Software;
        return device;
    }
}
