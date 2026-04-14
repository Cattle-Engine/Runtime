#include <SDL3/SDL.h>
#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderers/sdl_gpu_renderer.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/core.hpp"

namespace CE::Renderer::SDL_GPU_Renderer::Utils {
    SDL_GPUShader* LoadShader(
        SDL_GPUDevice* device,
        const std::string& shaderfilename,
        Uint32 samplercount,
        Uint32 uniformbuffercount,
        Uint32 storagebuffercount,
        Uint32 storagetexturecount,
        const std::string& basePath,
        CE::VFS::VFS* vfs
    ) {
        if (vfs == nullptr) {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] VFS is null");
            return nullptr;
        }

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
        auto* shaderFile = vfs->V_fopen(fullPath.c_str(), "rb");
        if (!shaderFile) {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] Failed to open shader file: {}", fullPath);
            return nullptr;
        }

        // Get file size
        uint64_t fileSize = 0;
        if (!vfs->GetFileSize(fullPath.c_str(), fileSize)) {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] Failed to get shader file size: {}", fullPath);
            vfs->V_fclose(shaderFile);
            return nullptr;
        }

        // Read shader data
        std::vector<uint8_t> shaderData(fileSize);
        size_t bytesRead = vfs->V_fread(shaderData.data(), 1, fileSize, shaderFile);
        vfs->V_fclose(shaderFile);

        if (bytesRead != fileSize) {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] Failed to read shader file: {}", fullPath);
            return nullptr;
        }

        SDL_GPUShaderCreateInfo shaderinfo{};
        shaderinfo.code_size = static_cast<size_t>(fileSize);
        shaderinfo.code = shaderData.data();
        shaderinfo.entrypoint = entrypoint;
        shaderinfo.format = format;
        shaderinfo.stage = stage;
        shaderinfo.num_samplers = samplercount;
        shaderinfo.num_storage_textures = storagetexturecount;
        shaderinfo.num_storage_buffers = storagebuffercount;
        shaderinfo.num_uniform_buffers = uniformbuffercount;

        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderinfo);
        if (shader == nullptr) {
            CE::Log(LogLevel::Error, "[Renderer Utils] [LoadShader] Failed to create GPU shader: {}", SDL_GetError());
            return nullptr;
        }

        CE::Log(LogLevel::Info, "[Renderer Utils] [LoadShader] Successfully loaded shader: {}", fullPath);
        return shader;
    }

    glm::mat4 GetView(const Camera2D& cam) {
        glm::mat4 view(1.0f);

        view = glm::translate(view, glm::vec3(-cam.x, -cam.y, 0.0f));
        view = glm::scale(view, glm::vec3(cam.zoom, cam.zoom, 1.0f));
        return view;
    }

    glm::mat4 GetProjection(float width, float height) {
    return glm::ortho(
            0.0f, width,
            height, 0.0f,   // flipped top left.
            -1.0f, 1.0f
        );
    }

    glm::mat4 GetCameraMatrix(const Camera2D& cam, float w, float h) {
        glm::mat4 proj = GetProjection(w, h);
        glm::mat4 view = GetView(cam);

        return proj * view;
    }
}
