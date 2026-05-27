#include "engine/rendering/renderers/software_renderer.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::Software {
    void Software_Renderer::LogAbout3D() {
        CE::Log(LogLevel::Warn, "[Software Renderer] 3D is not supported in the software renderer");
    }

    GPUMesh* Software_Renderer::CreateGPUMesh(MeshData& mesh) {
        (void)mesh;
        LogAbout3D();
        return nullptr;
    }

    void Software_Renderer::DestroyGPUMesh(GPUMesh* mesh) {
        (void)mesh;
        LogAbout3D();
    }

    void Software_Renderer::DrawMesh(GPUMesh* mesh, Material& material, const Transform3D& transform, bool error_tex) {
        (void)mesh;
        (void)material;
        (void)transform;
        (void)error_tex;
        LogAbout3D();
    }

    void Software_Renderer::ChangeCameraPos3D(const Transform3D& transform) {
        (void)transform;
        LogAbout3D();
    }

    void Software_Renderer::BeginMode3D() {}
    void Software_Renderer::EndMode3D() {}
}