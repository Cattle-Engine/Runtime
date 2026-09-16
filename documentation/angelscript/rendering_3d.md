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

Sets the camera 3d and updates the position
