#include <utility>

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

    ASCubemap::ASCubemap(CE::Renderer::Resources::TextureManager& texman) : mTextureManager(texman) {}

    ASCubemap::ASCubemap(
                CE::Renderer::Resources::TextureManager& texman,
                const TexHandle& left, const TexHandle& right,
                const TexHandle& top, const TexHandle& bottom,
                const TexHandle& front, const TexHandle& back
    ) : mTextureManager(texman) {
        SetCubemapFace(left, Faces::Left);
        SetCubemapFace(right, Faces::Right);
        SetCubemapFace(top, Faces::Bottom);
        SetCubemapFace(bottom, Faces::Bottom);
        SetCubemapFace(front, Faces::Front);
        SetCubemapFace(back, Faces::Back);
    }

    void ASCubemap::SetCubemapFace(const TexHandle& handle, Faces face) {
        if (handle == InvalidHandle) {
            mTextureRefs[static_cast<int>(face)].Reset();
            
            switch (face) {
                case Faces::Right: mCubemap.right = nullptr; break;
                case Faces::Left: mCubemap.left = nullptr; break;
                case Faces::Top: mCubemap.top = nullptr; break;
                case Faces::Bottom: mCubemap.bottom = nullptr; break;
                case Faces::Front: mCubemap.front = nullptr; break;
                case Faces::Back: mCubemap.back = nullptr; break;
            }
        }

        if (handle == mTextureRefs[static_cast<int>(face)].GetHandleID()) {
            CE_LOG(CE::LogLevel::Warn, "[Cubemap] Same handle detected face for: {}", FaceToString(face));
        }

        TexRef tex_ref = mTextureManager.Acquire(handle);

        if (!tex_ref.IsValid()) {
            CE_LOG(LogLevel::Error, "[Cubemap] Invalid texture handle");
        }

        mTextureRefs[static_cast<int>(face)] = std::move(tex_ref);

        switch (face) {
            case Faces::Right: mCubemap.right = mTextureRefs[static_cast<int>(face)].Get(); break;
            case Faces::Left: mCubemap.left = mTextureRefs[static_cast<int>(face)].Get(); break;
            case Faces::Top: mCubemap.top = mTextureRefs[static_cast<int>(face)].Get(); break;
            case Faces::Bottom: mCubemap.bottom = mTextureRefs[static_cast<int>(face)].Get(); break;
            case Faces::Front: mCubemap.front = mTextureRefs[static_cast<int>(face)].Get(); break;
            case Faces::Back: mCubemap.back = mTextureRefs[static_cast<int>(face)].Get(); break;
        }
    }
}