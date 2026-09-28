#include <SDL3/SDL_gpu.h>
#include "engine/common/tracelog.hpp"
#include "engine/rendering/renderers/sdl_gpu_renderer_new.hpp"

namespace CE::Renderer::SDL_GPU_Renderer {
    SDL_GPUShader* SDLGPURenderer::LoadShader(const std::string& shader_name, ShaderStage stage,uint32_t sampler_count, uint32_t uniform_buffer_count, uint32_t storage_buffer_count) {
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
            full_path = "Shaders/Compiled/SPRIV" + shader_name;
            format = SDL_GPU_SHADERFORMAT_SPIRV;
            entry_point = "main";
        } else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) {
            full_path = "Shaders/Compiled/MSL" + shader_name;
            format = SDL_GPU_SHADERFORMAT_MSL;
            entry_point = "main0"; 
        } else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) {
            full_path = "Compiled/DXIL/" + shader_name;
            format = SDL_GPU_SHADERFORMAT_DXIL;
            entry_point = "main";
        } else {
            CE_LOG(LogLevel::Error, "[SDLGPURenderer] Unknown shader backend format!");
            return nullptr;
        }

        auto file = mVFS.OpenFile(full_path);

        
    }
}