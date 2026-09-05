#include "engine/scripting/bindings/as_cubemap.hpp"
#include "engine/common/utils/enum_to_string.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Scripting::Bindings { 
    std::string ASCubemap::FaceToString(Faces face) {
        switch (face) {
            case Faces::Right:
                return CE::Utils::EnumToString<Faces::Right>();
            case Faces::Left:
                return CE::Utils::EnumToString<Faces::Left>();
            case Faces::Top:
                return CE::Utils::EnumToString<Faces::Top>();
            case Faces::Bottom:
                return CE::Utils::EnumToString<Faces::Bottom>();
            case Faces::Front:
                return CE::Utils::EnumToString<Faces::Front>();
            case Faces::Back:
                return CE::Utils::EnumToString<Faces::Back>();
        }

        return "Unknown";
    }

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
            CE_LOG(CE::LogLevel::Warn, "[Cubemap] Same handle detected for: {}", FaceToString(face));
        }
    }
}