(**Overview**)
- **Purpose:**: Detailed reference for writing and using shaders with the CE SDL_GPU renderer backend.
- **Files:**: See [sdl_gpu_shaders.cpp](source/engine/rendering/renderers/sdl_gpu/sdl_gpu_shaders.cpp#L1-L200) and [sdl_gpu_pipeline.cpp](source/engine/rendering/renderers/sdl_gpu/sdl_gpu_pipeline.cpp#L1-L200) for the implementation this doc describes.

**Shader Layout**
- **Stages:**: CE uses two shader stages: `vertex` (.vert) and `fragment` (.frag).
- **Naming:**: Shaders are referenced by a base name. The engine strips `.vert` / `.frag` when loading via `GetShaderBaseName()`; pass either a path with a suffix or a base path to `LoadShader()` and the engine will try `<base>.vert` and `<base>.frag`.

**How shaders are loaded and compiled**
- **Create:**: `CreateShaderProgram()` constructs an engine `Shader` whose backend handle is an `SDL_GPU_Renderer_Shader`.
- **Load stages:**: `LoadShaderStage()` / `LoadShaderStageIntoProgram()` calls `Utils::LoadShader()` to load a single stage. For fragment shaders, a sampler count may be provided.
- **Defaults:**: You can make a stage use the engine default by calling `UseDefaultShaderStage()`. Default shaders loaded at startup are `standard_vertex.vert` and `standard_fragment.frag` (see `CreateDefaultPipeline`).
- **Compile:**: `CompileShaderProgram()` ensures both stages exist and then creates a graphics pipeline via `CreateGraphicsPipeline()` (or `Create3DGraphicsPipeline()` when the shader is in 3D mode). Successful compile produces a `SDL_GPUGraphicsPipeline` stored in `program->Pipeline`.

**Pipeline Modes (2D vs 3D)**
- **Auto-detect by path:**: If the shader path contains the substring "3d" (case-insensitive) `ShaderPathSuggests3D()` sets the program `Mode` to `Mode3D` during load.
- **Manual override:**: Setting integer uniforms named `render3d`, `pipeline3d`, or `mode3d` to non-zero will switch `Mode3D` at runtime. A mode change marks the program `Dirty` and requires recompilation before use.

**Vertex input / attributes**
- **Vertex layout:**: The renderer expects the `Vertex` structure and declares three attributes:
	- location 0: `vec3` position (`FLOAT3`) at offset 0
	- location 1: normalized `ubyte4` color at offset `sizeof(float) * 3`
	- location 2: `vec2` texcoord at offset `sizeof(float) * 3 + sizeof(uint8_t) * 4`

**Uniforms and binding conventions**
This renderer normalizes uniform names to lowercase before matching (see `NormalizeUniformName()`) — all comparisons are case-insensitive.

- **Push / uniform buffers:**: The engine pushes uniforms in a fixed layout:
	- Vertex uniform buffer slot 0: `mat4 mvp` (the global or overridden MVP).
	- Vertex uniform buffer slot 1: `VertexShaderUserData` (only pushed if the program doesn't use the default vertex shader).
	- Fragment uniform buffer slot 0: `FragmentShaderUserData` (pushed for every draw).

	The C++ structs used (from `sdl_gpu_pipeline.cpp`) are:

	- `VertexShaderUserData` {
		- `mat4 model` (per-program model matrix)
		- `mat4 customMat4` (single custom 4x4)
		- `vec4 customVec4[8]` (array of 8 vec4 custom slots)
		- `ivec4 customInt4[4]` (array of 4 ivec4 custom slots)
		}

	- `FragmentShaderUserData` {
		- `vec4 tint` (default 1,1,1,1)
		- `vec4 resolution` (x,y = pixels or viewport, z/w reserved)
		- `vec4 misc` (used for `time`, `time2`, `time3`, `time4`)
		- `vec4 customVec4[8]`
		- `ivec4 customInt4[4]`
		- `vec4 sunDirectionEnabled` (xyz = sun direction, w = enabled)
		- `vec4 sunColourIntensity` (rgb = sun colour, a = intensity)
		- `vec4 ambientColourIntensity` (rgb = ambient colour, a = intensity)
		- `vec4 materialTint`
		- `vec4 materialProps` (x = roughness multiplier, y = metallic multiplier)
		- `vec4 cameraPositionShininess`
		- `vec4 normalExists` (x = 1 when a normal map is bound)
	}

- **Direct uniform setters (host-side helpers):** The renderer exposes functions that map specific uniform names to fields in the program object. These are the canonical names the engine expects:
	- Floats: `time`, `time2`, `time3`, `time4` → `program->Misc.x/y/z/w` (pushed to fragment `misc`).
	- Vectors:
		- `resolution` or `screensize` → `program->Resolution` (fragment)
		- `tint`, `colour`, `color` → `program->Tint` (fragment)
	- Matrices:
		- `mvp` → `program->OverrideMVP` and toggles `HasOverrideMVP` (vertex uniform slot 0 will use this instead of `gMVP`)
		- `model` → `program->ModelMatrix` (vertex user data)
		- `custommat4` → `program->CustomMat4` (vertex user data)

- **Indexed custom uniforms:**
	The engine supports indexed naming for arrays via simple decimal suffix parsing. The parser expects the name to start with the prefix and end with a non-empty decimal number; the number must be < `maxCount`.

	Conventions used by the host API functions:
	- `customvec4N` (N = 0..7 if `CustomVec4` length is 8): maps to `program->CustomVec4[N]` (vec4). Use `SetShaderVec2/Vec3/Vec4` or `SetShaderFloat` (via `customfloat`) as appropriate.
	- `customfloatN` (N indexes individual float components across the `CustomVec4[]` array): parsed with `maxCount = CustomVec4.size() * 4`. Mapping: `CustomVec4[index/4][index%4]`.
		- e.g. `customfloat0` → `CustomVec4[0].x`, `customfloat3` → `CustomVec4[0].w`, `customfloat4` → `CustomVec4[1].x`.
	- `customintN` (flat int indexing over `CustomInt4`): parsed with `maxCount = CustomInt4.size() * 4`, mapped into `CustomInt4[index/4][index%4]`.

	Implementation detail: indexed parsing is implemented in `TryParseSuffixIndex()` — it strips the prefix, calls `std::strtol()` and validates the parsed value and range.

**Textures & samplers**
- **Sampler count:** When loading a fragment shader you may pass a desired fragment sampler count. The engine uses a default minimum of 1. Internally `program->FragmentSamplerCount` holds the active number of samplers.
- **Binding textures:** Use `SetShaderTexture(name, texture, slot)` on the host side to bind a texture into `program->BoundTextures[slot]`. If the `slot` is >= current `BoundTextures.size()` the vector is resized. If `(slot+1)` exceeds `FragmentSamplerCount` the sampler count is increased.
- **Bind order at draw time:** `BindShaderSamplers()` constructs `samplerCount = max(1, program->FragmentSamplerCount)` bindings. Slot 0 is set to the draw texture/sampler by default; remaining slots are filled from `program->BoundTextures` (or `gWhiteTex`/`gWhiteSampler` fallback).

**Uniform buffer binding slots (summary)**
- Vertex buffer slot 0: `mvp` (mat4)
- Vertex buffer slot 1: `VertexShaderUserData` (model, customMat4, customVec4[], customInt4[])
- Fragment buffer slot 0: `FragmentShaderUserData` (2D user fields followed by 3D lighting/material fields)

**Shader authoring notes / best practices**
- **Names & case:**: All host-side uniform lookups are lowercased; prefer lowercase uniform names in your shaders to avoid surprises.
- **Reserved semantics:**: Use the engine-provided uniform names to interop with the host setters: `mvp`, `model`, `tint`, `resolution`, `time`, etc.
- **Fragment user block:**: Declare the complete `FragmentShaderUserData` layout in every fragment shader, including the 3D lighting/material fields, even when a shader does not consume them. This keeps the push-data layout stable across 2D and 3D pipelines.
- **Sampler indices:**: The host binds samplers starting at slot 0. If your shader expects multiple textures, index them in the shader sampler bindings starting at binding 0 (or the corresponding descriptor set if your shading language requires explicit bindings).
- **Matching vertex attributes:**: Ensure your vertex shader reads attributes at locations 0..2 with matching types/offsets (pos vec3, color ubyte4_norm, uv vec2) — mismatched layouts will produce incorrect rendering.
- **Mode switch:**: If your shader does 3D-specific work, include `3d` in the shader filename or call `SetShaderInt("mode3d", 1)` at runtime to force `Mode3D` so the engine will create the 3D pipeline variant.

**Runtime lifecycle & errors**
- **Dirty flag:**: Changing `Mode` or reloading a stage marks the program `Dirty`. `BindShader()` will attempt to compile the program if `Dirty` or the pipeline is missing.
- **Logging:**: Load/compile problems are logged via `CE_LOG` with useful error messages. If stage loading fails, the load function returns `nullptr` and the program is cleaned up.

**Examples**
- To create and use a shader program named `fx/wm` the engine will attempt to load `fx/wm.vert` and `fx/wm.frag`. After loading call `BindShader()` to set it active.
- To set the second custom vec4's y component from the host: call `SetShaderFloat("customfloat5", val)` which maps to `CustomVec4[1].y` (index 5 → vec index 1, component 1).
- To bind a texture to slot 2: `SetShaderTexture("u_tex2", myTex, 2)` — this will resize `BoundTextures` and increase `FragmentSamplerCount` if needed.

**Reference / implementation links**
- `TryParseSuffixIndex`, uniform normalization and host setters: [sdl_gpu_shaders.cpp](source/engine/rendering/renderers/sdl_gpu/sdl_gpu_shaders.cpp#L1-L200)
- Push layout and pipeline creation: [sdl_gpu_pipeline.cpp](source/engine/rendering/renderers/sdl_gpu/sdl_gpu_pipeline.cpp#L1-L200)
