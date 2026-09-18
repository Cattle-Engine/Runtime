# CE::Graphics::Render3D

## Functions
### SetSkybox
Return type: `void`

Signature:
```angelscript
const Cubemap& in skybox
```

Sets a Cubemap to be a skybox. 
If a cubemap has NoTexture inside a face that face will just use the default clear colour

### DrawModel
Return type: `void`

Signature:
```angelscript
const Model& in model, const Transform3D& in transform
```

Draws a model

### DestroyModel
Return type: `void`

Signature:
```angelscript
Model& in model
```

Easily destroys a model in one call

### LoadModel
Return type: `Model`

Signature:
```angelscript
const string& in model_path
```

Easily load a model (eg, gltf, fbx) from the VFS

### Set3DCameraPos
Return type: `void`

Signature:
```angelscript
const Transform3D& in transform
```

Sets the 3D camera position, rotation and scale

### SetCamera3D
Return type: `void`

Signature:
```angelscript
Camera3D& in camera
```

Sets the camera 3D and updates the position

### CompileMesh
Return type: `Mesh`

Signature:
```angelscript
MeshData& in mesh_data
```

Before you can draw a MeshData you must compile it into something the GPU understands

### DrawMesh
Return type: `void`

Signature:
```angelscript
const Mesh& in mesh, const Transform3D& in transform, const Material& in material, bool error_texture = false
```

Draws a mesh. If error_texture is set to true if material albedo is missing it uses the error texture

### DrawMesh
Return type: `void`

Signature:
```angelscript
const Mesh& in mesh, const Mat4& in transform, const Material& in material, bool error_texture = false
```

Draws a mesh, using mat4 as its position. If error_texture is set to true if material albedo is missing it uses the error texture

### DestroyMesh
Return type: `void`

Signature:
```angelscript
const Mesh& in mesh
```

Destroys a mesh
