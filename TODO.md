- [X] Make mouse lock & visbility avalaible in AngelScript
- [X] Make a small manager for GPUMesh
- [X] Create file for 3D primtives
- [X] Refactor to new texture manager

- [X] Add DrawMeshMat4 implimentation to SDL_GPU_Renderer, and a stub for software renderer
- [X] 3D model loading
- [X] 3D model drawing
- [X] Add support for metallic, roughness and normals as textures
- [ ]
- [ ] Update docs to show new AngelScript bindings
- [ ] Make glass materials actually work
- [ ] Multiple light support
- [X] Refactor AnimationManager to be AnimatedTextureManager
- [ ] Add AngelScript bindings for the new stuff
- [ ] Refactor shader manager to live in engine/rendering/resources and have it handle based
- [ ] Update AngelScript bindings for the shaders
- [ ] Basic 3D model bone support
- [ ] 3D model animation support
- [ ] AngelScript bindings for bones
- [ ] AngelScript imports:
// Main script
```angelscript
import test; // Imports a file called test, adds whatever has "export" inside the file
import test.foo_func; // Import just 1 function
```

Script export
```angelscript
export int FooInt = 100;

export int foo_func() {

}
```

- [ ] AngelScript attributes:
Basically a thing you can put on classes, structs and variables. Will look like this:
```angelscript
[foo_attribute]
struct foo {
    
}
```
- [ ] AngelScript #define, #ifdef and shish
- [ ] Make a UI system
- [ ] Add an ECS
- [ ] Physics via Jolt
- [ ] Save system
- [ ] plugins

- [ ] POLISH
- [ ] 1.0 ?