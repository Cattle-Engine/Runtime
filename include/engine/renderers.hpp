#pragma once

#include <memory>
#include <SDL3/SDL.h>

namespace CE::Renderers {
    enum class TextureFormat {
        RGBA8,
        RGB8,
    };

    struct Colour { // BRITISH SPELLING HEEEEEEEEEHHEHEHE
        int r;
        int g;
        int b;
    };

    struct Texture {
        void* handle;    
        int width;
        int height;
        TextureFormat format;
    };

    void Init(SDL_Window* window);
    void Update();
    std::shared_ptr<Texture> LoadTexture(const char* path);
    void DrawTexture(Texture texture);

    void Shutdown();
}