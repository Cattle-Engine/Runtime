#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

typedef struct float2 {
    float x, y;
} float2;

typedef struct FullscreenContext {
    SDL_GPUGraphicsPipeline* pipeline;

    float2 position;   // passed to vertex shader uniform
    int colorIndex;    // selects fragment color
} FullscreenContext;


// ----------------------------------------------------
// Helper: create shader (placeholder wrapper)
// ----------------------------------------------------
static SDL_GPUShader* CreateShader(
    SDL_GPUDevice* device,
    const char* path,
    SDL_GPUShaderStage stage,
    int vertexUniformCount,
    int fragmentUniformCount
) {
    SDL_GPUShaderCreateInfo info;
    SDL_zero(info);

    info.code_size = 0; // assume precompiled in real usage
    info.stage = stage;

    info.num_uniform_buffers = (stage == SDL_GPU_SHADERSTAGE_VERTEX)
        ? vertexUniformCount
        : fragmentUniformCount;

    return SDL_CreateGPUShader(device, &info);
}


// ----------------------------------------------------
// Pipeline creation
// ----------------------------------------------------
static SDL_GPUGraphicsPipeline* CreatePipeline(SDL_GPUDevice* device) {

    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_zero(info);

    info.vertex_shader =
        CreateShader(device, "FullscreenTriangle.vert",
                     SDL_GPU_SHADERSTAGE_VERTEX,
                     1, 0);

    info.fragment_shader =
        CreateShader(device, "FullscreenTriangle.frag",
                     SDL_GPU_SHADERSTAGE_FRAGMENT,
                     0, 1);

    SDL_GPUGraphicsPipeline* pipeline =
        SDL_CreateGPUGraphicsPipeline(device, &info);

    SDL_assert(pipeline);
    return pipeline;
}


// ----------------------------------------------------
// Init context
// ----------------------------------------------------
static FullscreenContext CreateFullscreenContext(SDL_GPUDevice* device) {
    FullscreenContext ctx;
    SDL_zero(ctx);

    ctx.pipeline = CreatePipeline(device);

    ctx.position.x = 0.5f;
    ctx.position.y = 0.5f;

    ctx.colorIndex = 0;

    return ctx;
}


// ----------------------------------------------------
// Input update
// ----------------------------------------------------
static void HandleInput(FullscreenContext* ctx, SDL_Event* event) {
    if (event->type == SDL_EVENT_KEY_DOWN) {
        switch (event->key.scancode) {
            case SDL_SCANCODE_1: ctx->colorIndex = 0; break;
            case SDL_SCANCODE_2: ctx->colorIndex = 1; break;
            case SDL_SCANCODE_3: ctx->colorIndex = 2; break;
            default: break;
        }
    }
}


// ----------------------------------------------------
// Update movement
// ----------------------------------------------------
static void Update(FullscreenContext* ctx, const bool* keys, float dt) {

    const float speed = 1.0f;

    if (keys[SDL_SCANCODE_D]) ctx->position.x += speed * dt;
    if (keys[SDL_SCANCODE_A]) ctx->position.x -= speed * dt;
    if (keys[SDL_SCANCODE_W]) ctx->position.y += speed * dt;
    if (keys[SDL_SCANCODE_S]) ctx->position.y -= speed * dt;
}


// ----------------------------------------------------
// Render
// ----------------------------------------------------
static void Draw(
    FullscreenContext* ctx,
    SDL_GPUCommandBuffer* cmd,
    SDL_GPURenderPass* pass
) {

    SDL_FColor colors[3] = {
        {1, 0, 0, 1},
        {0, 1, 0, 1},
        {0, 0, 1, 1}
    };

    SDL_BindGPUGraphicsPipeline(pass, ctx->pipeline);

    // Vertex shader uniform (float2 position)
    SDL_PushGPUVertexUniformData(
        cmd,
        0,
        &ctx->position,
        sizeof(float2)
    );

    // Fragment shader uniform (color)
    SDL_PushGPUFragmentUniformData(
        cmd,
        0,
        &colors[ctx->colorIndex],
        sizeof(SDL_FColor)
    );

    // 3 vertices = fullscreen triangle
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
}


// ----------------------------------------------------
// MAIN
// ----------------------------------------------------
int main(int argc, char** argv) {

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window =
        SDL_CreateWindow("Fullscreen Triangle", 1280, 720, 0);

    SDL_GPUDevice* device =
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, NULL);

    SDL_ClaimWindowForGPUDevice(device, window);

    FullscreenContext ctx = CreateFullscreenContext(device);

    const bool* keyState = SDL_GetKeyboardState(NULL);

    Uint64 lastTime = SDL_GetTicksNS();
    int running = 1;

    while (running) {

        Uint64 now = SDL_GetTicksNS();
        float dt = (now - lastTime) / 1000000000.0f;
        lastTime = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_EVENT_QUIT)
                running = 0;

            HandleInput(&ctx, &event);
        }

        Update(&ctx, keyState, dt);

        SDL_GPUCommandBuffer* cmd =
            SDL_AcquireGPUCommandBuffer(device);

        SDL_GPUTexture* swapchain =
            SDL_AcquireGPUSwapchainTexture(cmd, window);

        SDL_GPURenderPass* pass =
            SDL_BeginGPURenderPass(cmd, swapchain, NULL);

        Draw(&ctx, cmd, pass);

        SDL_EndGPURenderPass(pass);
        SDL_SubmitGPUCommandBuffer(cmd);
    }

    SDL_ReleaseGPUGraphicsPipeline(device, ctx.pipeline);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}