#pragma once

#include <array>

#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/texture_manager.hpp"

namespace CE::Scripting::Bindings {
    using TexRef = CE::Renderer::Resources::TextureRef;
    using TexHandle = CE::Renderer::Resources::TextureHandle;
    inline const TexHandle& InvalidHandle = CE::Renderer::Resources::kInvalidHandle;
    
    class ASCubemap {
        friend struct Face;
        public:
            ASCubemap(CE::Renderer::Resources::TextureManager& texman);
            ASCubemap(
                CE::Renderer::Resources::TextureManager& texman,
                const TexHandle& left, const TexHandle& right = InvalidHandle,
                const TexHandle& top = InvalidHandle, const TexHandle& bottom = InvalidHandle,
                const TexHandle& front = InvalidHandle, const TexHandle& back = InvalidHandle
            );

            enum class Faces {
                Right,
                Left,
                Top,
                Bottom,
                Front,
                Back
            };

            struct Face {
                ASCubemap& host;
                TexHandle handle;
                Faces face = Faces::Right;

                Face(ASCubemap& map, Faces init_face) : host(map), face(init_face) {}

                TexHandle GetTextureHandle() {
                    return handle;
                }

                Face& operator=(const TexHandle& in) {
                    handle = in;
                    host.SetCubemapFace(handle, face);
                    return *this;
                }

                Face& operator=(const Face& in) {
                    handle = in.handle;
                    host.SetCubemapFace(handle, face);
                    return *this;
                }
            };

            Face Right = Face(*this, Faces::Right);
            Face Left = Face(*this, Faces::Left);
            Face Top = Face(*this, Faces::Top);
            Face Bottom = Face(*this, Faces::Bottom);
            Face Front = Face(*this, Faces::Front);
            Face Back = Face(*this, Faces::Bottom);
        private:
            CE::Renderer::CubeMap mCubemap;
            CE::Renderer::Resources::TextureManager& mTextureManager;

            std::array<TexRef, 6> mTextureRefs;

            void SetCubemapFace(TexHandle& handle, Faces face);       
             std::string FaceToString(Faces face);
    };
}