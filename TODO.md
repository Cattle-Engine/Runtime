- [X] Make mouse lock & visbility avalaible in AngelScript
- [X] Make a small manager for GPUMesh
- [X] Create file for 3D primtives
- [X] Refactor to new texture manager
- [X] Add DrawMeshMat4 implimentation to SDL_GPU_Renderer, and a stub for software renderer
- [X] 3D model loading
- [X] 3D model drawing
- [X] Add support for metallic, roughness and normals as textures
- [X] Small alias thing for resource handles
- [X] Refactor AnimationManager to be AnimatedTextureManager
- [X] Refactor shader manager to live in engine/rendering/resources and have it handle based
- [X] Refactor everything else to use the new shader system
- [X] Add "ShaderRef" like TextureRef but for shaders (for the future material shaders)
- [X] Hook up the alias thing to instance
- [X] Hook up name registry stuff into AngelScript
- [X] Make CMake define CE_GIT_HASH,  CE_GIT_HASH_FULL, CE_GIT_BRANCH, CE_GIT_TAGS, CE_GIT_ISDIRTY
- [X] Make the logger use std::source_location in debug builds

- [ ] Make the AST for angelscript
- [ ] Make new module system
- [ ] Make the Runtime class actually use the new module system

- [ ] AngelScript imports:

Going to do symbol mangling so this.
(With the namespace hash it is generated from the full symbol)

This for a generated function
```__ce_mod_f_<moduleHash>_<namespaceHash>_<symbolHash>_<signatureHash>_<returnType>```

This for generated globals (vars)
```__ce_mod_g_<moduleHash>_<namespaceHash>_<symbolHash>_<typeHash>```

This for generated types
```__ce_mod_t_<moduleHash>_<namespaceHash>_<symbolHash>```

Internal stuff
```__ce_mod_i_<moduleHash>_<namespaceHash>_<symbolHash>_<signatureHash>```

// Main script
```angelscript
// This stuff is global like C++
import test; // Imports a file called test, adds whatever has "export" inside the file
import test::foo_func; // Import just 1 function

void main() {
    test::foo_func();
}
```

Script export
```angelscript
export int FooInt = 100;

export int foo_func() {

}
```

- [ ] Gdb style thing inside the debug window for angelscript. also lets you modify variables
- [ ] Refactor the angelscript bindings to have something like IScriptBinding
- [ ] Connect input binder to instance
- [X] Fix skybox not rendering when no mesh's are drawn
- [ ] Bindings for the input binder in angelscript
- [ ] Add support for setting shader/s on a material
- [ ] Emission textures
- [ ] HDR rendering
- [ ] Tone mapping
- [ ] Bloom
- [ ] Make glass materials actually work
- [ ] Multiple light support
- [ ] Update AngelScript bindings for the shaders
- [ ] Add XXHash to licence stuff
- [ ] Basic 3D model bone support
- [ ] 3D model animation support
- [ ] AngelScript bindings for bones
- [ ] In debug window have git commit info and in logs
- [ ] Update docs to show new AngelScript bindings

- [ ] AngelScript attributes:
Basically a thing you can put on classes, structs and variables. Will look like this:
```angelscript
@foo_attribute,@test_able
struct foo {
    
}
```
- [ ] AngelScript #define, #ifdef and shish
- [ ] Make a UI system
- [ ] Add an ECS
- [ ] Physics via Jolt
- [ ] Save system
- [ ] plugins
- [ ] In the angelscript bindings for plugins have it so you use the import keyword so like
```angelscript
import CE::Plugins::{plugin name here};
```

- [ ] POLISH
- [ ] 1.0 ?