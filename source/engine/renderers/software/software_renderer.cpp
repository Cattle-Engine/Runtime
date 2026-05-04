#include "engine/renderers/software_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include <SDL3_image/SDL_image.h>
#include "imgui/imgui.h"
#include "imgui_impl_sdl3.h"

#include "engine/common/sdl_events.hpp"

namespace {
    constexpr float kPi = 3.14159265358979323846f;

    SDL_PixelFormat ToSDLPixelFormat(CE::Renderer::TextureFormat format) {
        switch (format) {
            case CE::Renderer::TextureFormat::RGBA8:
                return SDL_PIXELFORMAT_RGBA32;
            case CE::Renderer::TextureFormat::RGB8:
                return SDL_PIXELFORMAT_RGB24;
            case CE::Renderer::TextureFormat::R8:
                return SDL_PIXELFORMAT_RGBA32;
            default:
                return SDL_PIXELFORMAT_RGBA32;
        }
    }

    int BytesPerPixel(CE::Renderer::TextureFormat format) {
        switch (format) {
            case CE::Renderer::TextureFormat::RGBA8:
                return 4;
            case CE::Renderer::TextureFormat::RGB8:
                return 3;
            case CE::Renderer::TextureFormat::R8:
                return 1;
            default:
                return 4;
        }
    }
}

namespace CE::Renderer::Software {
    Software_Renderer::Software_Renderer(VFS::VFS* vfs) : mVFS(vfs) {
    }

    Software_Renderer::~Software_Renderer() {
        Shutdown(nullptr);
    }

    void Software_Renderer::PreWinInit() {
    }

    int Software_Renderer::Init(SDL_Window* window, bool, GPUDeviceHandle gdevice) {
        if (gdevice == nullptr || gdevice->backend != RendererBackend::Software) {
            return 1;
        }

        if (mRenderer != nullptr) {
            return 0;
        }

        mRenderer = SDL_CreateRenderer(window, "software");;
        if (mRenderer == nullptr) {
            return 2;
        }

        SetVSync(mVSyncEnabled);
        ImGuiInit(window);

        const uint32_t whitePixel = 0xFFFFFFFFu;
        mWhiteTexture = CreateTextureFromData(
            1,
            1,
            &whitePixel,
            TextureFormat::RGBA8,
            static_cast<int>(sizeof(uint32_t)),
            TextureFilter::Nearest,
            TextureWrap::Clamp
        );

        uint32_t errorPixels[4] = {
            0xFFFF00FFu, 0xFF000000u,
            0xFF000000u, 0xFFFF00FFu
        };

        mErrorTexture = CreateTextureFromData(
            2,
            2,
            errorPixels,
            TextureFormat::RGBA8,
            2 * static_cast<int>(sizeof(uint32_t)),
            TextureFilter::Nearest,
            TextureWrap::Clamp
        );

        return (mWhiteTexture != nullptr && mErrorTexture != nullptr) ? 0 : 3;
    }

    int Software_Renderer::Shutdown(SDL_Window*) {
        if (mRenderer == nullptr && mOwnedTextures.empty() && mImGuiContext == nullptr) {
            return 0;
        }

        auto textures = mOwnedTextures;
        for (Texture* texture : textures) {
            DestroyTexture(texture);
        }
        mOwnedTextures.clear();
        mWhiteTexture = nullptr;
        mErrorTexture = nullptr;

        ImGuiShutdown();

        if (mRenderer != nullptr) {
            SDL_DestroyRenderer(mRenderer);
            mRenderer = nullptr;
        }

        return 0;
    }

    void Software_Renderer::ChangeCameraPos(float X, float Y, float zoom) {
        mCamera.x = X;
        mCamera.y = Y;
        mCamera.zoom = zoom <= 0.0f ? 1.0f : zoom;
    }

    SDL_FPoint Software_Renderer::ApplyCamera(float x, float y) const {
        return SDL_FPoint{
            (x - mCamera.x) * mCamera.zoom,
            (y - mCamera.y) * mCamera.zoom
        };
    }

    bool Software_Renderer::SetDrawColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return mRenderer != nullptr && SDL_SetRenderDrawColor(mRenderer, r, g, b, a);
    }

    void Software_Renderer::DrawRect(float x, float y, float w, float h,
                                     uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                                     float rotation) {
        DrawTex(
            mWhiteTexture,
            x,
            y,
            w,
            h,
            Colour{r, g, b, a},
            rotation
        );
    }

    void Software_Renderer::DrawCircle(float cx, float cy, float radius,
                                       int segments,
                                       uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (mRenderer == nullptr || radius <= 0.0f) {
            return;
        }

        const int segmentCount = std::max(segments, 12);
        const SDL_FPoint centre = ApplyCamera(cx, cy);
        const float scaledRadius = radius * mCamera.zoom;
        const float angleStep = (2.0f * kPi) / static_cast<float>(segmentCount);

        SDL_Vertex vertices[3];
        vertices[0].position = centre;
        vertices[0].color = SDL_FColor{
            r / 255.0f,
            g / 255.0f,
            b / 255.0f,
            a / 255.0f
        };
        vertices[0].tex_coord = SDL_FPoint{0.0f, 0.0f};

        for (int i = 0; i < segmentCount; ++i) {
            const float angle0 = angleStep * static_cast<float>(i);
            const float angle1 = angleStep * static_cast<float>(i + 1);

            vertices[1].position = SDL_FPoint{
                centre.x + std::cos(angle0) * scaledRadius,
                centre.y + std::sin(angle0) * scaledRadius
            };
            vertices[2].position = SDL_FPoint{
                centre.x + std::cos(angle1) * scaledRadius,
                centre.y + std::sin(angle1) * scaledRadius
            };
            vertices[1].color = vertices[0].color;
            vertices[2].color = vertices[0].color;
            vertices[1].tex_coord = SDL_FPoint{0.0f, 0.0f};
            vertices[2].tex_coord = SDL_FPoint{0.0f, 0.0f};

            SDL_RenderGeometry(mRenderer, nullptr, vertices, 3, nullptr, 0);
        }
    }

    void Software_Renderer::DrawLine(float x1, float y1, float x2, float y2,
                                     float thickness,
                                     uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (mRenderer == nullptr) {
            return;
        }

        const SDL_FPoint p1 = ApplyCamera(x1, y1);
        const SDL_FPoint p2 = ApplyCamera(x2, y2);
        const float scaledThickness = std::max(1.0f, thickness * mCamera.zoom);

        if (scaledThickness <= 1.0f) {
            if (SetDrawColour(r, g, b, a)) {
                SDL_RenderLine(mRenderer, p1.x, p1.y, p2.x, p2.y);
            }
            return;
        }

        const float dx = p2.x - p1.x;
        const float dy = p2.y - p1.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length <= 0.0f) {
            return;
        }

        const float nx = -dy / length;
        const float ny = dx / length;
        const float half = scaledThickness * 0.5f;

        SDL_Vertex verts[4];
        const SDL_FColor color{
            r / 255.0f,
            g / 255.0f,
            b / 255.0f,
            a / 255.0f
        };

        verts[0].position = SDL_FPoint{p1.x + nx * half, p1.y + ny * half};
        verts[1].position = SDL_FPoint{p1.x - nx * half, p1.y - ny * half};
        verts[2].position = SDL_FPoint{p2.x - nx * half, p2.y - ny * half};
        verts[3].position = SDL_FPoint{p2.x + nx * half, p2.y + ny * half};

        for (SDL_Vertex& vert : verts) {
            vert.color = color;
            vert.tex_coord = SDL_FPoint{0.0f, 0.0f};
        }

        const int indices[6] = {0, 1, 2, 0, 2, 3};
        SDL_RenderGeometry(mRenderer, nullptr, verts, 4, indices, 6);
    }

    void Software_Renderer::SetClearColor(float r, float g, float b, float a) {
        auto clamp = [](float value) -> uint8_t {
            return static_cast<uint8_t>(std::clamp(value, 0.0f, 255.0f));
        };

        mClearColor = SDL_Color{clamp(r), clamp(g), clamp(b), clamp(a)};
    }

    Texture* Software_Renderer::CreateTextureFromSurface(SDL_Surface* surface) {
        if (mRenderer == nullptr || surface == nullptr) {
            return nullptr;
        }

        SDL_Surface* rgbaSurface = surface;
        if (surface->format != SDL_PIXELFORMAT_RGBA32) {
            rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            if (rgbaSurface == nullptr) {
                return nullptr;
            }
        }

        SDL_Texture* sdlTexture = SDL_CreateTexture(
            mRenderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STATIC,
            rgbaSurface->w,
            rgbaSurface->h
        );

        if (sdlTexture == nullptr) {
            if (rgbaSurface != surface) {
                SDL_DestroySurface(rgbaSurface);
            }
            return nullptr;
        }

        SDL_SetTextureBlendMode(sdlTexture, SDL_BLENDMODE_BLEND);
        SDL_UpdateTexture(sdlTexture, nullptr, rgbaSurface->pixels, rgbaSurface->pitch);

        auto* data = new SoftwareTextureData{.texture = sdlTexture};
        auto* texture = new Texture{
            .handle = data,
            .width = rgbaSurface->w,
            .height = rgbaSurface->h,
            .format = TextureFormat::RGBA8,
            .backend = RendererBackend::Software
        };

        mOwnedTextures.insert(texture);

        if (rgbaSurface != surface) {
            SDL_DestroySurface(rgbaSurface);
        }

        return texture;
    }

    Texture* Software_Renderer::LoadTex(const char* path) {
        if (mRenderer == nullptr || path == nullptr || mVFS == nullptr) {
            return GetErrorTexture();
        }

        VirtualFile* file = mVFS->OpenFile(path);
        if (file == nullptr) {
            return GetErrorTexture();
        }

        if (file->sdl_stream == nullptr) {
            mVFS->CloseFile(file);
            return GetErrorTexture();
        }

        SDL_Surface* surface = IMG_Load_IO(file->sdl_stream, false);
        mVFS->CloseFile(file);
        if (surface == nullptr) {
            return GetErrorTexture();
        }

        Texture* texture = CreateTextureFromSurface(surface);
        SDL_DestroySurface(surface);
        return texture ? texture : GetErrorTexture();
    }

    Texture* Software_Renderer::CreateTextureFromData(int width,
                                                      int height,
                                                      const void* pixels,
                                                      TextureFormat format,
                                                      int pitch,
                                                      TextureFilter,
                                                      TextureWrap) {
        if (mRenderer == nullptr || pixels == nullptr || width <= 0 || height <= 0) {
            return nullptr;
        }

        const SDL_PixelFormat pixelFormat = ToSDLPixelFormat(format);
        SDL_Texture* sdlTexture = SDL_CreateTexture(
            mRenderer,
            pixelFormat,
            SDL_TEXTUREACCESS_STATIC,
            width,
            height
        );
        if (sdlTexture == nullptr) {
            return nullptr;
        }

        SDL_SetTextureBlendMode(sdlTexture, SDL_BLENDMODE_BLEND);

        const int resolvedPitch = pitch > 0 ? pitch : width * BytesPerPixel(format);
        const void* uploadPixels = pixels;
        int uploadPitch = resolvedPitch;
        std::vector<uint32_t> expandedPixels;

        if (format == TextureFormat::R8) {
            expandedPixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));

            for (int row = 0; row < height; ++row) {
                const auto* srcRow = static_cast<const uint8_t*>(pixels) + static_cast<size_t>(row) * static_cast<size_t>(resolvedPitch);
                for (int col = 0; col < width; ++col) {
                    const uint8_t value = srcRow[col];
                    expandedPixels[static_cast<size_t>(row) * static_cast<size_t>(width) + static_cast<size_t>(col)] =
                        0xFF000000u |
                        (static_cast<uint32_t>(value) << 16) |
                        (static_cast<uint32_t>(value) << 8) |
                        static_cast<uint32_t>(value);
                }
            }

            uploadPixels = expandedPixels.data();
            uploadPitch = width * static_cast<int>(sizeof(uint32_t));
        }

        if (!SDL_UpdateTexture(sdlTexture, nullptr, uploadPixels, uploadPitch)) {
            SDL_DestroyTexture(sdlTexture);
            return nullptr;
        }

        auto* data = new SoftwareTextureData{.texture = sdlTexture};
        auto* texture = new Texture{
            .handle = data,
            .width = width,
            .height = height,
            .format = format,
            .backend = RendererBackend::Software
        };

        mOwnedTextures.insert(texture);
        return texture;
    }

    bool Software_Renderer::RenderTexture(Texture* texture,
                                          float x, float y,
                                          float w, float h,
                                          const SDL_FRect* srcRect,
                                          Colour colour,
                                          float rotation) {
        if (mRenderer == nullptr || texture == nullptr || texture->handle == nullptr) {
            return false;
        }

        auto* data = static_cast<SoftwareTextureData*>(texture->handle);
        if (data->texture == nullptr) {
            return false;
        }

        const SDL_FPoint pos = ApplyCamera(x, y);
        SDL_FRect dstRect{
            pos.x,
            pos.y,
            w * mCamera.zoom,
            h * mCamera.zoom
        };

        SDL_SetTextureColorMod(data->texture, colour.r, colour.g, colour.b);
        SDL_SetTextureAlphaMod(data->texture, colour.a);

        const SDL_FPoint centre{
            dstRect.w * 0.5f,
            dstRect.h * 0.5f
        };

        return SDL_RenderTextureRotated(
            mRenderer,
            data->texture,
            srcRect,
            &dstRect,
            rotation,
            &centre,
            SDL_FLIP_NONE
        );
    }

    void Software_Renderer::DrawTex(Texture* texture, float x, float y,
                                    float w, float h, Colour colour,
                                    float rotation) {
        if (!RenderTexture(texture, x, y, w, h, nullptr, colour, rotation) && texture != GetErrorTexture()) {
            RenderTexture(GetErrorTexture(), x, y, w, h, nullptr, colour, rotation);
        }
    }

    void Software_Renderer::DrawTexUV(Texture* tex,
                                      float x, float y,
                                      float w, float h,
                                      float u0, float v0,
                                      float u1, float v1,
                                      Colour colour,
                                      float rotation) {
        if (tex == nullptr) {
            return;
        }

        SDL_FRect srcRect{
            std::min(u0, u1) * tex->width,
            std::min(v0, v1) * tex->height,
            std::abs(u1 - u0) * tex->width,
            std::abs(v1 - v0) * tex->height
        };

        if (srcRect.w <= 0.0f || srcRect.h <= 0.0f) {
            return;
        }

        if (!RenderTexture(tex, x, y, w, h, &srcRect, colour, rotation) && tex != GetErrorTexture()) {
            RenderTexture(GetErrorTexture(), x, y, w, h, nullptr, colour, rotation);
        }
    }

    void Software_Renderer::DestroyTexture(Texture* texture) {
        if (texture == nullptr) {
            return;
        }

        auto* data = static_cast<SoftwareTextureData*>(texture->handle);
        if (data != nullptr) {
            if (data->texture != nullptr) {
                SDL_DestroyTexture(data->texture);
            }
            delete data;
        }

        delete texture;
    }

    void Software_Renderer::UnloadTex(Texture* texture) {
        if (texture == nullptr) {
            return;
        }

        if (texture == mErrorTexture) {
            return;
        }

        if (mOwnedTextures.erase(texture) > 0) {
            DestroyTexture(texture);
        }
    }

    void Software_Renderer::DrawTriangle(float x0, float y0,
                                         float x1, float y1,
                                         float x2, float y2,
                                         uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                                         float) {
        if (mRenderer == nullptr) {
            return;
        }

        SDL_Vertex verts[3];
        const SDL_FColor color{
            r / 255.0f,
            g / 255.0f,
            b / 255.0f,
            a / 255.0f
        };

        verts[0].position = ApplyCamera(x0, y0);
        verts[1].position = ApplyCamera(x1, y1);
        verts[2].position = ApplyCamera(x2, y2);

        for (SDL_Vertex& vert : verts) {
            vert.color = color;
            vert.tex_coord = SDL_FPoint{0.0f, 0.0f};
        }

        SDL_RenderGeometry(mRenderer, nullptr, verts, 3, nullptr, 0);
    }

    void Software_Renderer::DrawRectLines(float x, float y, float w, float h,
                                          float thickness,
                                          uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        DrawLine(x, y, x + w, y, thickness, r, g, b, a);
        DrawLine(x + w, y, x + w, y + h, thickness, r, g, b, a);
        DrawLine(x + w, y + h, x, y + h, thickness, r, g, b, a);
        DrawLine(x, y + h, x, y, thickness, r, g, b, a);
    }

    void Software_Renderer::DrawCircleLines(float cx, float cy, float radius,
                                            int segments, float thickness,
                                            uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        const int segmentCount = std::max(segments, 12);
        const float angleStep = (2.0f * kPi) / static_cast<float>(segmentCount);

        for (int i = 0; i < segmentCount; ++i) {
            const float angle0 = angleStep * static_cast<float>(i);
            const float angle1 = angleStep * static_cast<float>(i + 1);

            DrawLine(
                cx + std::cos(angle0) * radius,
                cy + std::sin(angle0) * radius,
                cx + std::cos(angle1) * radius,
                cy + std::sin(angle1) * radius,
                thickness,
                r, g, b, a
            );
        }
    }

    int Software_Renderer::BeginFrame(SDL_Window*) {
        if (mRenderer == nullptr) {
            return 1;
        }

        if (!SDL_SetRenderDrawColor(mRenderer, mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a)) {
            return 2;
        }

        if (!SDL_RenderClear(mRenderer)) {
            return 3;
        }

        return 0;
    }

    int Software_Renderer::EndFrame(SDL_Window*) {
        if (mRenderer == nullptr) {
            return 1;
        }

        if (!ImGui::GetCurrentContext() && mImGuiContext != nullptr) {
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(mImGuiContext));
        }

        SDL_RenderPresent(mRenderer);
        return 0;
    }

    Texture* Software_Renderer::GetErrorTexture() {
        return mErrorTexture;
    }

    void* Software_Renderer::GetNativeTextureHandle(Texture* texture) {
        if (texture == nullptr || texture->handle == nullptr) {
            return nullptr;
        }

        auto* data = static_cast<SoftwareTextureData*>(texture->handle);
        return data->texture;
    }

    int Software_Renderer::Debug_GetVertCount() {
        return 0;
    }

    int Software_Renderer::Debug_GetIndexCount() {
        return 0;
    }

    int Software_Renderer::Debug_GetTexIndexCount() {
        return 0;
    }

    int Software_Renderer::Debug_GetTexVertCount() {
        return 0;
    }

    Camera2D* Software_Renderer::GetCamera() {
        return &mCamera;
    }

    void Software_Renderer::SetVSync(bool setting) {
        mVSyncEnabled = setting;
        if (mRenderer != nullptr) {
            SDL_SetRenderVSync(mRenderer, setting ? 1 : 0);
        }
    }

    void Software_Renderer::EnsureImGuiContext() {
        if (mImGuiContext == nullptr) {
            mImGuiContext = ImGui::CreateContext();
        }
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(mImGuiContext));
    }

    bool Software_Renderer::CreateImGuiFontTexture() {
        EnsureImGuiContext();

        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels = nullptr;
        int width = 0;
        int height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        if (pixels == nullptr || width <= 0 || height <= 0) {
            return false;
        }

        if (mImGuiFontTexture != nullptr) {
            UnloadTex(mImGuiFontTexture);
            mImGuiFontTexture = nullptr;
        }

        mImGuiFontTexture = CreateTextureFromData(
            width,
            height,
            pixels,
            TextureFormat::RGBA8,
            width * static_cast<int>(sizeof(uint32_t)),
            TextureFilter::Linear,
            TextureWrap::Clamp
        );

        if (mImGuiFontTexture == nullptr) {
            io.Fonts->SetTexID(ImTextureID_Invalid);
            return false;
        }

        io.Fonts->SetTexID(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(GetNativeTextureHandle(mImGuiFontTexture))));
        return true;
    }

    void Software_Renderer::DestroyImGuiFontTexture() {
        if (mImGuiContext != nullptr) {
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(mImGuiContext));
            ImGui::GetIO().Fonts->SetTexID(ImTextureID_Invalid);
        }

        if (mImGuiFontTexture != nullptr) {
            UnloadTex(mImGuiFontTexture);
            mImGuiFontTexture = nullptr;
        }
    }

    void Software_Renderer::ImGuiInit(SDL_Window* window) {
        EnsureImGuiContext();

        float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.BackendRendererUserData = this;
        io.BackendRendererName = "ce_software_sdl";

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(scale);
        style.FontScaleDpi = scale;

        io.Fonts->Clear();
        ImFontConfig fontCfg{};
        fontCfg.SizePixels = 16.0f * scale;
        io.FontDefault = io.Fonts->AddFontDefaultVector(&fontCfg);

        ImGui_ImplSDL3_InitForSDLRenderer(window, mRenderer);
        CreateImGuiFontTexture();
    }

    void Software_Renderer::ImGuiShutdown() {
        if (mImGuiContext == nullptr) {
            return;
        }

        ImGuiContext* prevContext = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(mImGuiContext));

        ImGuiIO& io = ImGui::GetIO();

        DestroyImGuiFontTexture();
        ImGui_ImplSDL3_Shutdown();
        io.BackendRendererName = nullptr;
        io.BackendRendererUserData = nullptr;
        io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset);
        ImGui::DestroyContext(static_cast<ImGuiContext*>(mImGuiContext));
        mImGuiContext = nullptr;

        ImGui::SetCurrentContext(prevContext);
    }

    void Software_Renderer::ImGuiStartFrame() {
        EnsureImGuiContext();

        auto indices = CE::SDL_Events::GetWindowEventIndices(SDL_GetWindowID(SDL_GetRenderWindow(mRenderer)));
        for (size_t i : indices) {
            const SDL_Event& e = CE::SDL_Events::gEvents[i];
            ImGui_ImplSDL3_ProcessEvent(&e);
        }

        if (ImGui::GetIO().Fonts->TexRef.GetTexID() == ImTextureID_Invalid) {
            CreateImGuiFontTexture();
        }

        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void Software_Renderer::RenderImGuiDrawData(ImDrawData* drawData) {
        if (mRenderer == nullptr || drawData == nullptr) {
            return;
        }

        const ImVec2 clipOff = drawData->DisplayPos;
        const ImVec2 clipScale = drawData->FramebufferScale;

        for (int n = 0; n < drawData->CmdListsCount; ++n) {
            const ImDrawList* cmdList = drawData->CmdLists[n];

            for (const ImDrawCmd& cmd : cmdList->CmdBuffer) {
                if (cmd.UserCallback != nullptr) {
                    if (cmd.UserCallback == ImDrawCallback_ResetRenderState) {
                        SDL_SetRenderClipRect(mRenderer, nullptr);
                    } else {
                        cmd.UserCallback(cmdList, &cmd);
                    }
                    continue;
                }

                ImVec2 clipMin(
                    (cmd.ClipRect.x - clipOff.x) * clipScale.x,
                    (cmd.ClipRect.y - clipOff.y) * clipScale.y
                );
                ImVec2 clipMax(
                    (cmd.ClipRect.z - clipOff.x) * clipScale.x,
                    (cmd.ClipRect.w - clipOff.y) * clipScale.y
                );

                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) {
                    continue;
                }

                SDL_Rect clipRect{
                    static_cast<int>(clipMin.x),
                    static_cast<int>(clipMin.y),
                    static_cast<int>(clipMax.x - clipMin.x),
                    static_cast<int>(clipMax.y - clipMin.y)
                };
                SDL_SetRenderClipRect(mRenderer, &clipRect);

                std::vector<SDL_Vertex> vertices;
                vertices.reserve(cmd.ElemCount);
                std::vector<int> indices;
                indices.reserve(cmd.ElemCount);

                for (unsigned int elem = 0; elem < cmd.ElemCount; ++elem) {
                    const ImDrawIdx idx = cmdList->IdxBuffer[cmd.IdxOffset + elem];
                    const ImDrawVert& src = cmdList->VtxBuffer[cmd.VtxOffset + idx];

                    SDL_Vertex vertex;
                    vertex.position.x = src.pos.x;
                    vertex.position.y = src.pos.y;
                    vertex.tex_coord.x = src.uv.x;
                    vertex.tex_coord.y = src.uv.y;

                    const ImU32 col = src.col;
                    vertex.color.r = ((col >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
                    vertex.color.g = ((col >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
                    vertex.color.b = ((col >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
                    vertex.color.a = ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;

                    indices.push_back(static_cast<int>(vertices.size()));
                    vertices.push_back(vertex);
                }

                SDL_Texture* texture = reinterpret_cast<SDL_Texture*>(static_cast<uintptr_t>(cmd.GetTexID()));
                SDL_RenderGeometry(mRenderer, texture, vertices.data(), static_cast<int>(vertices.size()), indices.data(), static_cast<int>(indices.size()));
            }
        }

        SDL_SetRenderClipRect(mRenderer, nullptr);
    }

    void Software_Renderer::ImGuiEndFrame(SDL_Window*) {
        EnsureImGuiContext();
        ImGui::Render();
        RenderImGuiDrawData(ImGui::GetDrawData());
    }
}
