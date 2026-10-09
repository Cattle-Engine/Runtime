#include <algorithm>

#include "engine/rendering/renderers/sdl_gpu_renderer.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    namespace {
        bool RequiresRepeatSampler(float u0, float v0, float u1, float v1) {
            constexpr float kEpsilon = 0.0001f;
            return std::min(u0, u1) < -kEpsilon || std::max(u0, u1) > 1.0f + kEpsilon || std::min(v0, v1) < -kEpsilon ||
                   std::max(v0, v1) > 1.0f + kEpsilon;
        }

        void RotatePoint(float& x, float& y, float cx, float cy, float sinA, float cosA) {
            float dx = x - cx;
            float dy = y - cy;
            x = cx + (dx * cosA) - (dy * sinA);
            y = cy + (dx * sinA) + (dy * cosA);
        }

        void UpdateTexBatchCounts(std::vector<TexVertexBatch>& batches, uint32_t currentVertCount, uint32_t currentIndexCount) {
            if (!batches.empty()) {
                batches.back().vertCount = currentVertCount - batches.back().vertOffset;
                batches.back().idxCount = currentIndexCount - batches.back().idxOffset;
            }
        }

        void UpdatePrimitiveBatchCounts(std::vector<PrimitiveBatch>& batches, uint32_t currentIndexCount) {
            if (!batches.empty()) {
                batches.back().idxCount = currentIndexCount - batches.back().idxOffset;
            }
        }
    }

    void SDL_GPU_Renderer::DrawSpriteUV(
        Texture* texture, 
        float x, float y, 
        float w, float h, 
        float u0, float v0, 
        float u1, float v1, 
        Colour colour, 
        float rotation, 
        TextureFlip flip
    ) {
        if (!mFrameActive) {
            if (!mWarnedOutsideFrame) {
                CE_LOG(LogLevel::Error, "[SDL_GPU renderer] Can't draw outside of draw begin frame!");
                mWarnedOutsideFrame = true;
            }
            return;
        }
        if (!texture || !texture->handle) return;

        auto* tex = static_cast<SDLGPUTexData*>(texture->handle);
        if (mTexVertCount + 4 > kMaxVertices || mTexIndexCount + 6 > kMaxIndices) return;

        SDL_GPUSampler* sampler = RequiresRepeatSampler(u0, v0, u1, v1) ? tex->repeatSampler : tex->sampler;

        if (mTexBatches.empty() || mCurrentTex != tex || mCurrentTexSampler != sampler ||
            mTexBatches.back().shader != mCurrentShader) {
            mTexBatches.push_back({tex, sampler, mCurrentShader, mTexVertCount, 0, mTexIndexCount, 0});
            mCurrentTex = tex;
            mCurrentTexSampler = sampler;
        }

        float cx = x + (w * 0.5f);
        float cy = y + (h * 0.5f);
        float sinA = std::sin(rotation);
        float cosA = std::cos(rotation);

        float px[4] = {x, x + w, x + w, x};
        float py[4] = {y, y, y + h, y + h};
        float pu[4] = {u0, u1, u1, u0};
        float pv[4] = {v0, v0, v1, v1};

        if (HasTextureFlip(flip, TextureFlip::Horizontal)) {
            std::swap(pu[0], pu[1]);
            std::swap(pu[3], pu[2]);
        }
        if (HasTextureFlip(flip, TextureFlip::Vertical)) {
            std::swap(pv[0], pv[3]);
            std::swap(pv[1], pv[2]);
        }

        uint16_t base = (uint16_t)mTexVertCount;
        uint8_t r = colour.r;
        uint8_t g = colour.g;
        uint8_t b = colour.b;
        uint8_t a = colour.a;

        for (int i = 0; i < 4; i++) {
            RotatePoint(px[i], py[i], cx, cy, sinA, cosA);
            mMappedTexVerts[base + i] = {px[i], py[i], 0, r, g, b, a, pu[i], pv[i]};
        }

        mMappedTexIndices[mTexIndexCount + 0] = base;
        mMappedTexIndices[mTexIndexCount + 1] = base + 1;
        mMappedTexIndices[mTexIndexCount + 2] = base + 2;
        mMappedTexIndices[mTexIndexCount + 3] = base + 2;
        mMappedTexIndices[mTexIndexCount + 4] = base + 3;
        mMappedTexIndices[mTexIndexCount + 5] = base;

        mTexIndexCount += 6;
        mTexVertCount += 4;

        UpdateTexBatchCounts(mTexBatches, mTexVertCount, mTexIndexCount);
    }

    void SDL_GPU_Renderer::DrawCircleLines(
        float cx, float cy, 
        float radius, int segments, 
        float thickness, 
        uint8_t r, uint8_t g, uint8_t b, uint8_t a
    ) {
        if (!mFrameActive) {
            if (!mWarnedOutsideFrame) {
                CE_LOG(LogLevel::Error, "[SDL_GPU renderer] Can't draw outside of draw begin frame!");
                mWarnedOutsideFrame = true;
            }
            return;
        }
        segments = std::max(segments, 3);
        float step = (float)(2.0 * M_PI) / (float)segments;
        for (int i = 0; i < segments; ++i) {
            float a0 = step * (float)i;
            float a1 = step * (float)(i + 1);
            DrawLine(
                cx + (std::cos(a0) * radius), 
                cy + (std::sin(a0) * radius), 
                cx + (std::cos(a1) * radius),
                cy + (std::sin(a1) * radius), thickness, r, g, b, a
            );
        }
    }

    void SDL_GPU_Renderer::DrawCircle(float cx, float cy, float radius, int segments, uint8_t r, uint8_t g, uint8_t b,
                                      uint8_t a) {
        if (!mFrameActive) {
            if (!mWarnedOutsideFrame) {
                CE_LOG(LogLevel::Error, "[SDL_GPU renderer] Can't draw outside of draw begin frame!");
                mWarnedOutsideFrame = true;
            }
            return;
        }
        segments = std::max(segments, 3);

        uint32_t vNeeded = (uint32_t)(segments + 1); // centre + rim
        uint32_t iNeeded = (uint32_t)(segments * 3);

        if (mVertCount + vNeeded > kMaxVertices || mIndexCount + iNeeded > kMaxIndices) {
            CE_LOG(LogLevel::Warn, "[SDL_GPU Renderer] Batch full, skipping circle");
            return;
        }

        if (mPrimitiveBatches.empty() || mCurrentPrimitiveShader != mCurrentShader) {
            mPrimitiveBatches.push_back({mCurrentShader, mIndexCount, 0});
            mCurrentPrimitiveShader = mCurrentShader;
        }

        uint16_t centre = (uint16_t)mVertCount;
        mMappedVerts[centre] = {cx, cy, 0, r, g, b, a, 0.5f, 0.5f};
        mVertCount++;

        float step = (float)(2.0 * M_PI) / (float)segments;
        for (int i = 0; i < segments; ++i) {
            float angle = step * (float)i;
            float vx = cx + (std::cos(angle) * radius);
            float vy = cy + (std::sin(angle) * radius);
            float u = (std::cos(angle) + 1.0f) * 0.5f;
            float v = (std::sin(angle) + 1.0f) * 0.5f;
            mMappedVerts[mVertCount + i] = {vx, vy, 0, r, g, b, a, u, v};
        }

        for (int i = 0; i < segments; ++i) {
            uint16_t cur = (uint16_t)(mVertCount + i);
            uint16_t next = (uint16_t)(mVertCount + ((i + 1) % segments));
            mMappedIndices[mIndexCount++] = centre;
            mMappedIndices[mIndexCount++] = cur;
            mMappedIndices[mIndexCount++] = next;
        }

        mVertCount += (uint32_t)segments;
        UpdatePrimitiveBatchCounts(mPrimitiveBatches, mIndexCount);
    }

    void SDL_GPU_Renderer::DrawRect(
        float x, float y, 
        float w, float h, 
        uint8_t r, uint8_t g, uint8_t b, uint8_t a,
        float rotation
    ) {
        if (!mFrameActive) {
            if (!mWarnedOutsideFrame) {
                CE_LOG(LogLevel::Error, "[SDL_GPU renderer] Can't draw outside of draw begin frame!");
                mWarnedOutsideFrame = true;
            }
            return;
        }
        if (mVertCount + 4 > kMaxVertices || mIndexCount + 6 > kMaxIndices)
            return;

        if (mPrimitiveBatches.empty() || mCurrentPrimitiveShader != mCurrentShader) {
            mPrimitiveBatches.push_back({mCurrentShader, mIndexCount, 0});
            mCurrentPrimitiveShader = mCurrentShader;
        }

        float cx = x + (w * 0.5f);
        float cy = y + (h * 0.5f);
        float sinA = std::sin(rotation);
        float cosA = std::cos(rotation);

        float px[4] = {x, x + w, x + w, x};
        float py[4] = {y, y, y + h, y + h};

        uint16_t base = (uint16_t)mVertCount;
        for (int i = 0; i < 4; i++) {
            RotatePoint(px[i], py[i], cx, cy, sinA, cosA);
            mMappedVerts[base + i] = {px[i], py[i], 0, r, g, b, a, 0, 0};
        }

        mMappedIndices[mIndexCount++] = base;
        mMappedIndices[mIndexCount++] = base + 1;
        mMappedIndices[mIndexCount++] = base + 2;
        mMappedIndices[mIndexCount++] = base + 2;
        mMappedIndices[mIndexCount++] = base + 3;
        mMappedIndices[mIndexCount++] = base;
        mVertCount += 4;
        UpdatePrimitiveBatchCounts(mPrimitiveBatches, mIndexCount);
    }


    void SDL_GPU_Renderer::DrawRectLines(
        float x, float y, 
        float w, float h, 
        float thickness, 
        uint8_t r, uint8_t g, uint8_t b, uint8_t a
    ) {
        if (!mFrameActive) {
            if (!mWarnedOutsideFrame) {
                CE_LOG(LogLevel::Error, "[SDL_GPU renderer] Can't draw outside of draw begin frame!");
                mWarnedOutsideFrame = true;
            }
            return;
        }
        DrawLine(x, y, x + w, y, thickness, r, g, b, a);         // top
        DrawLine(x + w, y, x + w, y + h, thickness, r, g, b, a); // right
        DrawLine(x + w, y + h, x, y + h, thickness, r, g, b, a); // bottom
        DrawLine(x, y + h, x, y, thickness, r, g, b, a);         // left
    }

    void SDL_GPU_Renderer::DrawTriangle(
        float x0, float y0, 
        float x1, float y1, 
        float x2, float y2, 
        uint8_t r, uint8_t g, uint8_t b, uint8_t a, 
        float rotation
    ) {
        if (!mFrameActive) {
            if (!mWarnedOutsideFrame) {
                CE_LOG(LogLevel::Error, "[SDL_GPU renderer] Can't draw outside of draw begin frame!");
                mWarnedOutsideFrame = true;
            }
            return;
        }
        if (mVertCount + 3 > kMaxVertices || mIndexCount + 3 > kMaxIndices)
            return;

        if (mPrimitiveBatches.empty() || mCurrentPrimitiveShader != mCurrentShader) {
            mPrimitiveBatches.push_back({mCurrentShader, mIndexCount, 0});
            mCurrentPrimitiveShader = mCurrentShader;
        }

        // Centroid is the average of the three vertices
        float cx = (x0 + x1 + x2) / 3.0f;
        float cy = (y0 + y1 + y2) / 3.0f;
        float sinA = std::sin(rotation);
        float cosA = std::cos(rotation);

        float px[3] = {x0, x1, x2};
        float py[3] = {y0, y1, y2};

        uint16_t base = (uint16_t)mVertCount;
        for (int i = 0; i < 3; i++) {
            RotatePoint(px[i], py[i], cx, cy, sinA, cosA);
            mMappedVerts[base + i] = {px[i], py[i], 0, r, g, b, a, 0, 0};
        }

        mMappedIndices[mIndexCount++] = base;
        mMappedIndices[mIndexCount++] = base + 1;
        mMappedIndices[mIndexCount++] = base + 2;

        mVertCount += 3;
        UpdatePrimitiveBatchCounts(mPrimitiveBatches, mIndexCount);
    }


    void SDL_GPU_Renderer::DrawLine(
        float x1, float y1, 
        float x2, float y2, 
        float thickness, 
        uint8_t r, uint8_t g, uint8_t b, uint8_t a
    ) {
        if (!mFrameActive) {
            if (!mWarnedOutsideFrame) {
                CE_LOG(LogLevel::Error, "[SDL_GPU renderer] Can't draw outside of draw begin frame!");
                mWarnedOutsideFrame = true;
            }
            return;
        }
        if (mVertCount + 4 > kMaxVertices || mIndexCount + 6 > kMaxIndices) {
            CE_LOG(LogLevel::Warn, "[SDL_GPU Renderer] Batch full, skipping line");
            return;
        }

        if (mPrimitiveBatches.empty() || mCurrentPrimitiveShader != mCurrentShader) {
            mPrimitiveBatches.push_back({mCurrentShader, mIndexCount, 0});
            mCurrentPrimitiveShader = mCurrentShader;
        }

        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = std::sqrt((dx * dx) + (dy * dy));
        if (len < 1e-6f)
            return;

        // Perpendicular unit vector scaled to half-thickness
        float nx = (-dy / len) * (thickness * 0.5f);
        float ny = (dx / len) * (thickness * 0.5f);

        uint16_t base = (uint16_t)mVertCount;

        mMappedVerts[base + 0] = {x1 + nx, y1 + ny, 0, r, g, b, a, 0, 0};
        mMappedVerts[base + 1] = {x2 + nx, y2 + ny, 0, r, g, b, a, 0, 0};
        mMappedVerts[base + 2] = {x2 - nx, y2 - ny, 0, r, g, b, a, 0, 0};
        mMappedVerts[base + 3] = {x1 - nx, y1 - ny, 0, r, g, b, a, 0, 0};

        mMappedIndices[mIndexCount++] = base;
        mMappedIndices[mIndexCount++] = base + 1;
        mMappedIndices[mIndexCount++] = base + 2;
        mMappedIndices[mIndexCount++] = base + 2;
        mMappedIndices[mIndexCount++] = base + 3;
        mMappedIndices[mIndexCount++] = base;

        mVertCount += 4;
        UpdatePrimitiveBatchCounts(mPrimitiveBatches, mIndexCount);
    }

    void SDL_GPU_Renderer::DrawSprite(
        Texture* texture, 
        float x, float y, 
        float w, float h, 
        Colour colour, 
        float rotation,
        TextureFlip flip
    ) {
        if (!mFrameActive) {
            if (!mWarnedOutsideFrame) {
                CE_LOG(LogLevel::Error, "[SDL_GPU renderer] Can't draw outside of draw begin frame!");
                mWarnedOutsideFrame = true;
            }
            return;
        }
        if (!texture || !texture->handle)
            return;
        auto* tex = static_cast<SDLGPUTexData*>(texture->handle);
        if (mTexVertCount + 4 > kMaxVertices || mTexIndexCount + 6 > kMaxIndices)
            return;
        SDL_GPUSampler* sampler = tex->sampler;

        if (mTexBatches.empty() || mCurrentTex != tex || mCurrentTexSampler != sampler ||
            mTexBatches.back().shader != mCurrentShader) {
            mTexBatches.push_back({tex, sampler, mCurrentShader, mTexVertCount, 0, mTexIndexCount, 0});
            mCurrentTex = tex;
            mCurrentTexSampler = sampler;
        }

        float cx = x + (w * 0.5f);
        float cy = y + (h * 0.5f);
        float sinA = std::sin(rotation);
        float cosA = std::cos(rotation);

        float px[4] = {x, x + w, x + w, x};
        float py[4] = {y, y, y + h, y + h};
        float pu[4] = {0, 1, 1, 0};
        float pv[4] = {0, 0, 1, 1};

        if (HasTextureFlip(flip, TextureFlip::Horizontal)) {
            std::swap(pu[0], pu[1]);
            std::swap(pu[3], pu[2]);
        }
        if (HasTextureFlip(flip, TextureFlip::Vertical)) {
            std::swap(pv[0], pv[3]);
            std::swap(pv[1], pv[2]);
        }

        uint16_t base = (uint16_t)mTexVertCount;
        uint8_t r = colour.r;
        uint8_t g = colour.g;
        uint8_t b = colour.b;
        uint8_t a = colour.a;

        for (int i = 0; i < 4; i++) {
            RotatePoint(px[i], py[i], cx, cy, sinA, cosA);
            mMappedTexVerts[base + i] = {px[i], py[i], 0, r, g, b, a, pu[i], pv[i]};
        }

        mMappedTexIndices[mTexIndexCount + 0] = base;
        mMappedTexIndices[mTexIndexCount + 1] = base + 1;
        mMappedTexIndices[mTexIndexCount + 2] = base + 2;
        mMappedTexIndices[mTexIndexCount + 3] = base + 2;
        mMappedTexIndices[mTexIndexCount + 4] = base + 3;
        mMappedTexIndices[mTexIndexCount + 5] = base;

        mTexIndexCount += 6;
        mTexVertCount += 4;

        UpdateTexBatchCounts(mTexBatches, mTexVertCount, mTexIndexCount);
    }

    void SDL_GPU_Renderer::ChangeCameraPos2D(float X, float Y, float zoom) {
        mCamera2D = {X, Y, zoom};
    }

    Camera2D* SDL_GPU_Renderer::GetCamera() {
        return &mCamera2D;
    }
}