#include <SDL3/SDL.h>
#include <string>
#include <memory>

#include "engine/renderer.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/core.hpp"

namespace CE::Renderer::Utils {
    void MakeIdentity(float* m) {
        for (int i = 0; i < 16; i++) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    void MakeTranslate(float* m, float x, float y) {
        MakeIdentity(m);
        m[3] = x;
        m[7] = y;
    }

    SDL_GPUShader* LoadShader(
        SDL_GPUDevice* device,
        const std::string& shaderfilename,
        Uint32 samplercount,
        Uint32 uniformbuffercount,
        Uint32 storagebuffercount,
        Uint32 storagetexturecount,
        const std::string& basePath
    ) {
        SDL_GPUShaderStage stage;
        if (shaderfilename.find(".vert") != std::string::npos) {
            stage = SDL_GPU_SHADERSTAGE_VERTEX;
        } else if (shaderfilename.find(".frag") != std::string::npos){
            stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        } else {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] Invalid shader stage!");
            return nullptr;
        }

        SDL_GPUShaderFormat backendformats = SDL_GetGPUShaderFormats(device);

        SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
        const char* entrypoint = nullptr;
        std::string fullPath;

        // Select appropriate shader format and path based on backend support
        if (backendformats & SDL_GPU_SHADERFORMAT_SPIRV) {
            fullPath = basePath + "Compiled/SPIRV/" + shaderfilename + ".spv";
            format = SDL_GPU_SHADERFORMAT_SPIRV;
            entrypoint = "main";
        } else if (backendformats & SDL_GPU_SHADERFORMAT_MSL) {
            fullPath = basePath + "Compiled/MSL/" + shaderfilename + ".msl";
            format = SDL_GPU_SHADERFORMAT_MSL;
            entrypoint = "main0";
        } else if (backendformats & SDL_GPU_SHADERFORMAT_DXIL) {
            fullPath = basePath + "Compiled/DXIL/" + shaderfilename + ".dxil";
            format = SDL_GPU_SHADERFORMAT_DXIL;
            entrypoint = "main";
        } else {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] Unrecognized backend shader format!");
            return nullptr;
        }

        // Load shader data from VFS
        CE::VFS::VFS& vfs = CE::Global::GetVFS();
        auto* shaderFile = vfs.V_fopen(fullPath.c_str(), "rb");
        if (!shaderFile) {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] Failed to open shader file: {}", fullPath);
            return nullptr;
        }

        // Get file size
        uint64_t fileSize = 0;
        if (!vfs.GetFileSize(fullPath.c_str(), fileSize)) {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] Failed to get shader file size: {}", fullPath);
            vfs.V_fclose(shaderFile);
            return nullptr;
        }

        // Read shader data
        std::vector<uint8_t> shaderData(fileSize);
        size_t bytesRead = vfs.V_fread(shaderData.data(), 1, fileSize, shaderFile);
        vfs.V_fclose(shaderFile);

        if (bytesRead != fileSize) {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] Failed to read shader file: {}", fullPath);
            return nullptr;
        }


        SDL_GPUShaderCreateInfo shaderinfo = {
            .code_size = static_cast<size_t>(fileSize),
            .code = shaderData.data(),
            .entrypoint = entrypoint,
            .format = format,
            .stage = stage,
            .num_samplers = samplercount,
            .num_storage_textures = storagetexturecount,
            .num_storage_buffers = storagebuffercount,
            .num_uniform_buffers = uniformbuffercount
        };

        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderinfo);
        if (shader == nullptr) {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] Failed to create GPU shader: {}", SDL_GetError());
            return nullptr;
        }

        CE::Log(LogLevel::Info, "[Renderer Utils] [LoadShader] Successfully loaded shader: {}", fullPath);
        return shader;
    }
}