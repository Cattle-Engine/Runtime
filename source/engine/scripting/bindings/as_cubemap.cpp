#include "engine/scripting/bindings/as_cubemap.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Scripting::Bindings {
    /*
    * TODO: FINISH THESE
    */
    ASCubemap::ASCubemap(CE::Renderer::Resources::TextureManager& texman) : mTextureManager(texman) {

    }
    ASCubemap::ASCubemap(
                CE::Renderer::Resources::TextureManager& texman,
                const TexHandle& left, const TexHandle& right,
                const TexHandle& top, const TexHandle& bottom,
                const TexHandle& front, const TexHandle& back
    ) : mTextureManager(texman) {

    }

    void ASCubemap::SetCubemapFace(TexHandle& handle, Faces face) {
        if (handle == mTextureRefs[static_cast<int>(face)].GetHandleID()) {
            CE_LOG(CE::LogLevel::Warn, "[Cubemap] Same handle detected for");
        }
    }
}