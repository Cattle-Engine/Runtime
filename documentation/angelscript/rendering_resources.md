# CE::Graphics

## Functions
### CreateMaterial
Return type: `void`

Signature:
```angelscript
Material& out handle, const Texture& in texture
```

No description given

### DestroyMaterial
Return type: `void`

Signature:
```angelscript
const Material& in handle
```

No description given

### SetMaterialAlbedo
Return type: `void`

Signature:
```angelscript
const Material& in handle, const Texture& in texture
```

No description given

### SetMaterialTint
Return type: `void`

Signature:
```angelscript
const Material& in handle, const Colour& in colour
```

No description given

### SetMaterialRoughness
Return type: `void`

Signature:
```angelscript
const Material& in handle, const float& in roughness
```

No description given

### SetMaterialMetallic
Return type: `void`

Signature:
```angelscript
const Material& in handle, const float& in metallic
```

No description given

### MaterialExists
Return type: `bool`

Signature:
```angelscript
const Material& in handle
```

No description given

### LoadTexture
Return type: `CE::Texture`

Signature:
```angelscript
const string& in path
```

No description given

### UnloadTexture
Return type: `void`

Signature:
```angelscript
const Texture& in texture
```

No description given

### TextureExists
Return type: `bool`

Signature:
```angelscript
const Texture& in texture
```

No description given

### CreateProgram
Return type: `void`

Signature:
```angelscript
Shader& out shader
```

No description given

### LoadShader
Return type: `Shader`

Signature:
```angelscript
const string& in filepath, int fragment_sampler_count = 4
```

No description given

### LoadStage
Return type: `bool`

Signature:
```angelscript
const Shader& in, const string& in, const ShaderStage& in, int sampler_count = 4
```

No description given

### UseDefaultStage
Return type: `bool`

Signature:
```angelscript
const Shader& in, const ShaderStage& in
```

No description given

### CompileShader
Return type: `bool`

Signature:
```angelscript
const Shader& in
```

No description given

### BindShader
Return type: `bool`

Signature:
```angelscript
const Shader& in
```

No description given

### UnbindShader
Return type: `void`

No description given

### ShaderExists
Return type: `bool`

Signature:
```angelscript
const Shader& in
```

No description given

### UnloadShader
Return type: `void`

Signature:
```angelscript
const Shader& in
```

No description given

### UnloadAllShaders
Return type: `void`

No description given

### SetFloat
Return type: `void`

Signature:
```angelscript
const string& in, float
```

No description given

### SetVec2
Return type: `void`

Signature:
```angelscript
const string& in, const Vec2& in
```

No description given

### SetVec3
Return type: `void`

Signature:
```angelscript
const string& in, const Vec3& in
```

No description given

### SetVec4
Return type: `void`

Signature:
```angelscript
const string& in, const Vec4& in
```

No description given

### SetMat4
Return type: `void`

Signature:
```angelscript
const string& in, const Mat4& in
```

No description given

### SetInt
Return type: `void`

Signature:
```angelscript
const string& in, const int
```

No description given

### SetTexture
Return type: `bool`

Signature:
```angelscript
const string& in, const Texture& in, int slot = 0
```

No description given

### CreateMesh
Return type: `void`

Signature:
```angelscript
Mesh& out mesh, MeshData& in meshData
```

No description given

### DrawMesh
Return type: `void`

Signature:
```angelscript
const Mesh& in mesh, const Transform3D& in transform, const Material& in material, bool error_texture = false
```

No description given

### DrawMeshMat4
Return type: `void`

Signature:
```angelscript
const Mesh& in mesh, const Mat4& in transform, const Material& in material, bool error_texture = false
```

No description given

### ChangeMesh
Return type: `void`

Signature:
```angelscript
const Mesh& in mesh, MeshData& in meshData
```

No description given

### DestroyMesh
Return type: `void`

Signature:
```angelscript
const Mesh& in mesh
```

No description given

### MeshExists
Return type: `bool`

Signature:
```angelscript
const Mesh& in mesh
```

No description given
