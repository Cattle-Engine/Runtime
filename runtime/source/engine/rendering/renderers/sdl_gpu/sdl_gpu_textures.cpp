#include <SDL3/SDL_gpu.h>
#include <SDL3_image/SDL_image.h>

#include "engine/rendering/renderers/sdl_gpu_renderer.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    namespace {
        SDL_GPUSampler* CreateSampler(
            SDL_GPUDevice* device, 
            SDL_GPUFilter filter,
            SDL_GPUSamplerAddressMode addressMode
        ) {
            SDL_GPUSamplerCreateInfo sampInfo{};
            sampInfo.min_filter = filter;
            sampInfo.mag_filter = filter;
            sampInfo.address_mode_u = addressMode;
            sampInfo.address_mode_v = addressMode;
            sampInfo.address_mode_w = addressMode;
            sampInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
            sampInfo.min_lod = 0.0f;
            sampInfo.max_lod = 0.0f;
            return SDL_CreateGPUSampler(device, &sampInfo);
        }
    }

    Texture* SDL_GPU_Renderer::LoadTex(const char* path) {
        CE::Common::FS::VFS::VFS& vfs = *mVFS;

        uint64_t sz = 0;
        if (!vfs.GetFileSize(path, sz) || sz == 0) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] VFS could not stat '{}' (missing or empty)", path);
            return nullptr;
        }

        auto vf = vfs.OpenFile(path);
        if (!vf) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] VFS could not open '{}'", path);
            return nullptr;
        }

        std::vector<uint8_t> fileBytes((size_t)sz);
        if (!vf->Read(fileBytes.data(), fileBytes.size())) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] VFS could not read '{}'", path);
            return nullptr;
        }

        SDL_IOStream* mem = SDL_IOFromConstMem(fileBytes.data(), fileBytes.size());
        if (!mem) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] SDL_IOFromConstMem failed: {}", SDL_GetError());
            return nullptr;
        }

        SDL_Surface* surface = IMG_Load_IO(mem, true);
        if (!surface) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] IMG_Load_IO failed for '{}': {}", path, SDL_GetError());
            return nullptr;
        }

        SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!converted) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] SDL_ConvertSurface failed: {}", SDL_GetError());
            return nullptr;
        }

        int w = converted->w;
        int h = converted->h;
        size_t dataSize = static_cast<size_t>(w * h * 4);

        SDL_GPUTextureCreateInfo texInfo{};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texInfo.width = (Uint32)w;
        texInfo.height = (Uint32)h;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;

        SDL_GPUTexture* gpuTex = SDL_CreateGPUTexture(mDevice, &texInfo);
        if (!gpuTex) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] SDL_CreateGPUTexture failed: {}", SDL_GetError());
            SDL_DestroySurface(converted);
            return nullptr;
        }

        SDL_GPUTransferBufferCreateInfo tbInfo{};
        tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbInfo.size = (Uint32)dataSize;

        SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(mDevice, &tbInfo);
        void* mapped = SDL_MapGPUTransferBuffer(mDevice, tb, false);
        SDL_memcpy(mapped, converted->pixels, dataSize);
        SDL_UnmapGPUTransferBuffer(mDevice, tb);
        SDL_DestroySurface(converted);

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(mDevice);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTextureTransferInfo src{};
        src.transfer_buffer = tb;
        src.offset = 0;
        src.pixels_per_row = (Uint32)w;
        src.rows_per_layer = (Uint32)h;

        SDL_GPUTextureRegion dst{};
        dst.texture = gpuTex;
        dst.w = (Uint32)w;
        dst.h = (Uint32)h;
        dst.d = 1;

        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(mDevice, tb);

        SDLGPUTexData* data = new SDLGPUTexData();
        data->gpuTex = gpuTex;
        data->sampler = CreateSampler(mDevice, SDL_GPU_FILTER_LINEAR, SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE);
        data->repeatSampler = CreateSampler(mDevice, SDL_GPU_FILTER_LINEAR, SDL_GPU_SAMPLERADDRESSMODE_REPEAT);
        if (!data->sampler || !data->repeatSampler) {
            if (data->sampler) {
                SDL_ReleaseGPUSampler(mDevice, data->sampler);
            }
            if (data->repeatSampler) {
                SDL_ReleaseGPUSampler(mDevice, data->repeatSampler);
            }
            SDL_ReleaseGPUTexture(mDevice, gpuTex);
            delete data;
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] Failed to create texture samplers: {}", SDL_GetError());
            return nullptr;
        }

        Texture* tex = new Texture();
        tex->handle = data;
        tex->width = w;
        tex->height = h;
        tex->format = TextureFormat::RGBA8;
        tex->backend = mBackend;
        return tex;
    }

    Texture* SDL_GPU_Renderer::CreateTextureFromData(
        int width, int height, 
        const void* pixels, 
        TextureFormat format,
        int pitch, 
        TextureFilter filter, TextureWrap wrap,
        TextureUploadBatch* batch
    ) {
        if (!mDevice) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] CreateTextureFromData called before Init()");
            return nullptr;
        }

        if (width <= 0 || height <= 0) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] CreateTextureFromData invalid size {}x{}", width, height);
            return nullptr;
        }

        if (!pixels) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] CreateTextureFromData pixels was null");
            return nullptr;
        }

        auto bytesPerPixelFor = [](TextureFormat fmt) -> int {
            switch (fmt) {
            case TextureFormat::RGBA8:
                return 4;
            case TextureFormat::RGB8:
                return 3;
            case TextureFormat::R8:
                return 1;
            default:
                return 0;
            }
        };

        const int srcBpp = bytesPerPixelFor(format);
        if (srcBpp == 0) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] CreateTextureFromData unsupported format {}", (int)format);
            return nullptr;
        }

        const int srcPitchBytes = (pitch > 0) ? pitch : (width * srcBpp);
        if (srcPitchBytes < width * srcBpp) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] CreateTextureFromData pitch {} too small for {}x{}x{}",
                   srcPitchBytes, width, height, srcBpp);
            return nullptr;
        }
        if ((srcPitchBytes % srcBpp) != 0) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] CreateTextureFromData pitch {} not divisible by bpp {}",
                   srcPitchBytes, srcBpp);
            return nullptr;
        }

        const Uint32 srcPixelsPerRow = (Uint32)(srcPitchBytes / srcBpp);

        std::vector<uint8_t> converted;
        const uint8_t* uploadPtr = static_cast<const uint8_t*>(pixels);
        Uint32 uploadPixelsPerRow = srcPixelsPerRow;
        Uint32 uploadWidth = (Uint32)width;
        Uint32 uploadHeight = (Uint32)height;

        SDL_GPUTextureFormat gpuFormat = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

        if (format != TextureFormat::RGBA8) {
            converted.resize((size_t)width * (size_t)height * 4);
            const uint8_t* src = static_cast<const uint8_t*>(pixels);

            for (int row = 0; row < height; ++row) {
                const uint8_t* srcRow = src + ((size_t)row * (size_t)srcPitchBytes);
                uint8_t* dstRow = converted.data() + ((size_t)row * (size_t)width * 4);

                if (format == TextureFormat::RGB8) {
                    for (int col = 0; col < width; ++col) {
                        const uint8_t* s = srcRow + ((size_t)col * 3);
                        uint8_t* d = dstRow + ((size_t)col * 4);
                        d[0] = s[0];
                        d[1] = s[1];
                        d[2] = s[2];
                        d[3] = 255;
                    }
                } else {
                    for (int col = 0; col < width; ++col) {
                        const uint8_t v = srcRow[col];
                        uint8_t* d = dstRow + ((size_t)col * 4);
                        d[0] = v;
                        d[1] = v;
                        d[2] = v;
                        d[3] = 255;
                    }
                }
            }

            uploadPtr = converted.data();
            uploadPixelsPerRow = (Uint32)width;
        }

        SDL_GPUTextureCreateInfo texInfo{};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = gpuFormat;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texInfo.width = uploadWidth;
        texInfo.height = uploadHeight;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;

        SDL_GPUTexture* gpuTex = SDL_CreateGPUTexture(mDevice, &texInfo);
        if (!gpuTex) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] SDL_CreateGPUTexture failed: {}", SDL_GetError());
            return nullptr;
        }

        const size_t uploadBytesPerRow = (size_t)uploadPixelsPerRow * 4;
        const size_t uploadSize = uploadBytesPerRow * (size_t)height;

        SDL_GPUTransferBufferCreateInfo tbInfo{};
        tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbInfo.size = (Uint32)uploadSize;

        SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(mDevice, &tbInfo);
        if (!tb) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] SDL_CreateGPUTransferBuffer failed: {}", SDL_GetError());
            SDL_ReleaseGPUTexture(mDevice, gpuTex);
            return nullptr;
        }

        void* mapped = SDL_MapGPUTransferBuffer(mDevice, tb, false);
        if (!mapped) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] SDL_MapGPUTransferBuffer failed: {}", SDL_GetError());
            SDL_ReleaseGPUTransferBuffer(mDevice, tb);
            SDL_ReleaseGPUTexture(mDevice, gpuTex);
            return nullptr;
        }

        SDL_memcpy(mapped, uploadPtr, uploadSize);
        SDL_UnmapGPUTransferBuffer(mDevice, tb);

        SDLTextureUploadBatchData* batchData = batch ? static_cast<SDLTextureUploadBatchData*>(batch->handle) : nullptr;

        SDL_GPUCommandBuffer* cmd = batchData ? batchData->cmd : SDL_AcquireGPUCommandBuffer(mDevice);

        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTextureTransferInfo src{};
        src.transfer_buffer = tb;
        src.offset = 0;
        src.pixels_per_row = uploadPixelsPerRow;
        src.rows_per_layer = (Uint32)height;

        SDL_GPUTextureRegion dst{};
        dst.texture = gpuTex;
        dst.w = uploadWidth;
        dst.h = uploadHeight;
        dst.d = 1;

        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);

        if (batchData) {
            batchData->pendingTBs.push_back(tb);
        } else {
            SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);

            if (!fence) {
                CE_LOG(LogLevel::Error,
                    "[SDL_GPU Renderer] Failed to acquire GPU fence: {}",
                    SDL_GetError());

                SDL_ReleaseGPUTransferBuffer(mDevice, tb);
                return nullptr;
            }

            if (!SDL_WaitForGPUFences(mDevice, true, &fence, 1)) {
                CE_LOG(LogLevel::Error,
                    "[SDL_GPU Renderer] Failed waiting for GPU fence: {}",
                    SDL_GetError());
            }

            SDL_ReleaseGPUFence(mDevice, fence);
            SDL_ReleaseGPUTransferBuffer(mDevice, tb);
        }

        const SDL_GPUFilter gpuFilter =
            (filter == TextureFilter::Nearest) ? SDL_GPU_FILTER_NEAREST : SDL_GPU_FILTER_LINEAR;
        SDL_GPUSamplerAddressMode addressMode = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        switch (wrap) {
        case TextureWrap::Clamp:
            addressMode = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            break;
        case TextureWrap::Repeat:
            addressMode = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
            break;
        case TextureWrap::MirroredRepeat:
            addressMode = SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
            break;
        }

        SDLGPUTexData* data = new SDLGPUTexData();
        data->gpuTex = gpuTex;
        data->sampler = CreateSampler(mDevice, gpuFilter, addressMode);
        data->repeatSampler = CreateSampler(mDevice, gpuFilter, SDL_GPU_SAMPLERADDRESSMODE_REPEAT);

        if (!data->sampler || !data->repeatSampler) {
            CE_LOG(LogLevel::Error, "[SDL_GPU Renderer] SDL_CreateGPUSampler failed: {}", SDL_GetError());
            if (data->sampler)
                SDL_ReleaseGPUSampler(mDevice, data->sampler);
            if (data->repeatSampler)
                SDL_ReleaseGPUSampler(mDevice, data->repeatSampler);
            SDL_ReleaseGPUTexture(mDevice, gpuTex);
            delete data;
            return nullptr;
        }

        Texture* tex = new Texture();
        tex->handle = data;
        tex->width = width;
        tex->height = height;
        tex->format = TextureFormat::RGBA8;
        tex->backend = mBackend;
        return tex;
    }


    TextureUploadBatch* SDL_GPU_Renderer::BeginBatchTextureUpload() {
        auto* batchData = new SDLTextureUploadBatchData();
        batchData->cmd = SDL_AcquireGPUCommandBuffer(mDevice);

        auto* batch = new TextureUploadBatch();
        batch->handle = batchData;
        return batch;
    }

    void SDL_GPU_Renderer::EndBatchTextureUpload(TextureUploadBatch* batch) {
        if (!batch || !batch->handle) return;
        auto* data = static_cast<SDLTextureUploadBatchData*>(batch->handle);

        SDL_SubmitGPUCommandBuffer(data->cmd);
        for (auto* tb : data->pendingTBs)
            SDL_ReleaseGPUTransferBuffer(mDevice, tb);

        delete data;
        delete batch;
    }


    void SDL_GPU_Renderer::UnloadTex(Texture* texture) {
        if (!texture)
            return;
        if (texture->handle) {
            auto* data = static_cast<SDLGPUTexData*>(texture->handle);
            mDeferredDeletes.push_back({data, 3});
        }
        delete texture;
    }

    Texture* SDL_GPU_Renderer::GetErrorTexture() {
        static Texture errorTexture;

        static SDLGPUTexData data;
        data.gpuTex = mErrorTex;
        data.sampler = mErrorSampler;
        data.repeatSampler = mErrorSampler;

        errorTexture.handle = &data;
        errorTexture.width = 8;
        errorTexture.height = 8;
        errorTexture.format = TextureFormat::RGBA8;
        errorTexture.backend = RendererBackend::Vulkan;

        return &errorTexture;
    }

    void* SDL_GPU_Renderer::GetNativeTextureHandle(Texture* texture) {
        if (!texture || !texture->handle) {
            return nullptr;
        }

        auto* data = static_cast<SDLGPUTexData*>(texture->handle);
        return data ? data->gpuTex : nullptr;
    }


    void SDL_GPU_Renderer::ProcessDeferredDeletions(bool force) {
        if (mDeferredDeletes.empty())
            return;
        std::vector<DeferredDeleteEntry> entriesToDelete;

        for (auto& entry : mDeferredDeletes) {
            if (force) {
                entry.framesUntilDelete = 0;
            } else {
                entry.framesUntilDelete--;
            }

            if (entry.framesUntilDelete <= 0) {
                if (entry.data->sampler)
                    SDL_ReleaseGPUSampler(mDevice, entry.data->sampler);
                if (entry.data->repeatSampler && entry.data->repeatSampler != entry.data->sampler) {
                    SDL_ReleaseGPUSampler(mDevice, entry.data->repeatSampler);
                }
                if (entry.data->gpuTex)
                    SDL_ReleaseGPUTexture(mDevice, entry.data->gpuTex);
                delete entry.data;
                entriesToDelete.push_back(entry);
            }
        }

        for (const auto& entry : entriesToDelete) {
            mDeferredDeletes.erase(std::remove(mDeferredDeletes.begin(), mDeferredDeletes.end(), entry),
                                   mDeferredDeletes.end());
        }
    }
}