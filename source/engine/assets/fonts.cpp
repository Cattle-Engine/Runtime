#include <string>
#include <unordered_map>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL.h>

#include "engine/renderer.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/assets/default_font.hpp"
#include "engine/common/vfs.hpp"
#include "engine/assets/fonts.hpp"

namespace CE::Assets::Fonts {
    FontManager::FontManager(Renderer::IRenderer& renderer, VFS::VFS& vfs, uint64_t instance_id)
        : mRenderer(renderer),
        mVFS(vfs),
        mInstanceID(instance_id)
    {
        TTF_Init();

        SDL_IOStream* stream = SDL_IOFromMem(
            Default::default_ce_font,
            Default::default_ce_font_len
        );

        mFallBackFont = TTF_OpenFontIO(stream, 1, 24);
        mDefaultFont = mFallBackFont;
    }

    void FontManager::Draw(std::string text, int x, int y, float size , Renderer::Colour colour){
        if (TTF_GetFontSize(mDefaultFont) != size) {
            TTF_SetFontSize(mDefaultFont, size);
        }

        SDL_Color sdlColor = {
            colour.r,
            colour.g,
            colour.b,
            colour.a
        };
        SDL_Surface* surface = TTF_RenderText_Blended(
            mDefaultFont,
            text.c_str(),
            0,
            sdlColor
        );

        if (!surface) return;

        auto texture = mRenderer.CreateTextureFromData(
            surface->w,
            surface->h,
            surface->pixels,
            Renderer::TextureFormat::RGBA8,
            surface->pitch
        );

        mRenderer.DrawTex(
            texture, 
            static_cast<float>(x), 
            static_cast<float>(y), 
            static_cast<float>(surface->w), 
            static_cast<float>(surface->h), 
            colour, 0.
        );

        mRenderer.UnloadTex(texture);
        SDL_DestroySurface(surface);
    }

    void FontManager::DrawEx(std::string text, std::string name ,int x, int y, float size , Renderer::Colour colour) {
        auto font = mFonts.find(name);

        if (font != mFonts.end()) {
            if (TTF_GetFontSize(font->second) != size) {
                TTF_SetFontSize(font->second, size);
            }
            SDL_Color sdlColor = {
                colour.r,
                colour.g,
                colour.b,
                colour.a
            };
            SDL_Surface* surface = TTF_RenderText_Blended(
                font->second,
                text.c_str(),
                0,
                sdlColor
            );
            if (!surface) return;
            auto texture = mRenderer.CreateTextureFromData(
                surface->w,
                surface->h,
                surface->pixels,
                Renderer::TextureFormat::RGBA8,
                surface->pitch
            );
            mRenderer.DrawTex(
                texture, 
                static_cast<float>(x), 
                static_cast<float>(y), 
                static_cast<float>(surface->w), 
                static_cast<float>(surface->h), 
                colour, 0.
            );
            mRenderer.UnloadTex(texture);
            SDL_DestroySurface(surface);
        } else {
            CE::Log(LogLevel::Error, "[Font Manager {}] Tried to draw with an unloaded font: ", mInstanceID, font->first);
            if (TTF_GetFontSize(mFallBackFont) != size) {
                TTF_SetFontSize(mFallBackFont, size);
            }
            SDL_Color sdlColor = {
                colour.r,
                colour.g,
                colour.b,
                colour.a
            };
            SDL_Surface* surface = TTF_RenderText_Blended(
                mFallBackFont,
                text.c_str(),
                0,
                sdlColor
            );
            if (!surface) return;
            auto texture = mRenderer.CreateTextureFromData(
                surface->w,
                surface->h,
                surface->pixels,
                Renderer::TextureFormat::RGBA8,
                surface->pitch
            );
            mRenderer.DrawTex(
                texture, 
                static_cast<float>(x), 
                static_cast<float>(y), 
                static_cast<float>(surface->w), 
                static_cast<float>(surface->h), 
                colour, 0.
            );
            mRenderer.UnloadTex(texture);
            SDL_DestroySurface(surface);
        }
    }

    void FontManager::Load(std::string path, std::string name) {
        TTF_Font* font;
        if(!mVFS.FileExists(path.c_str())) {
            CE::Log(LogLevel::Error, "[Font Manager {}] Missing font: ", mInstanceID, path);
            font = mFallBackFont;
        } else {
            VirtualFile* file = mVFS.OpenFile(path.c_str());
            font = TTF_OpenFontIO(file->sdl_stream, 1, 24);
        }
        mFonts[name] = font;
    }
}