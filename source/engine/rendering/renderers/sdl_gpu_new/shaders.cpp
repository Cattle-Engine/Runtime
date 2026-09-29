#include <memory>

#include <SDL3/SDL_gpu.h>
#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    detail::Shader SDLGPURenderer::LoadShader(const std::string& shader_name, ShaderStage stage, uint32_t sampler_count, uint32_t uniform_buffer_count, uint32_t storage_buffer_count, uint32_t storage_texture_count) {
        SDL_GPUShaderStage sdl_stage;

        if (stage == ShaderStage::Vertex) {
            sdl_stage = SDL_GPU_SHADERSTAGE_VERTEX;
        } else if (stage == ShaderStage::Fragment) {
            sdl_stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        }

        SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(mGPUDevice);
        SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
        std::string full_path;

        std::string entry_point;

        if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
            full_path = "shaders/Compiled/SPRIV" + shader_name;
            format = SDL_GPU_SHADERFORMAT_SPIRV;
            entry_point = "main";
        } else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) {
            full_path = "shaders/Compiled/MSL" + shader_name;
            format = SDL_GPU_SHADERFORMAT_MSL;
            entry_point = "main0"; 
        } else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) {
            full_path = "shaders/Compiled/DXIL/" + shader_name;
            format = SDL_GPU_SHADERFORMAT_DXIL;
            entry_point = "main";
        } else {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Unknown shader backend format!");
            return Shader();
        }

        auto file = mVFS.OpenFile(full_path);

        if (!file) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to open shader file: {}", full_path);
            return Shader();
        }        

        uint64_t file_size = 0;
        if (!mVFS.GetFileSize(full_path, file_size)) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to get shader file size: {}", full_path);
            return Shader();
        }

        std::unique_ptr<uint8_t[]> shader_data = std::make_unique<uint8_t[]>(file_size);

        const bool read_ok = file->Read(shader_data.get(), file_size);

        if (!read_ok) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to read shader data");
            return Shader();
        }

        SDL_GPUShaderCreateInfo shader_info{
            .code_size = static_cast<size_t>(file_size),
            .code = shader_data.get(),
            .entrypoint = entry_point.c_str(),
            .format = format,
            .stage = sdl_stage,
            .num_samplers = sampler_count,
            .num_storage_textures = storage_texture_count,
            .num_storage_buffers = storage_buffer_count,
            .num_uniform_buffers = uniform_buffer_count,
            .props = 0
        };

        SDL_GPUShader* shader = SDL_CreateGPUShader(mGPUDevice, &shader_info);
        if (shader == nullptr) {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Failed to create shader: '{}', {}", full_path,SDL_GetError());
            return Shader();
        }

        return Shader(mGPUDevice, shader);
    }
}