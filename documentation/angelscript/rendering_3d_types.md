# CE::Graphics::Render3D

Registration priority: `90`

## CubemapFace
C++ type: `CE::Scripting::Bindings::ASCubemap::Face`

Flags: `Reference`, `NoCount`

No description given

### Methods
#### GetTexture
Return type: `Texture`

No description given

### Operators
#### =
Return type: `CubemapFace&`

Signature:
```angelscript
const Texture& in
```

No description given

#### =
Return type: `CubemapFace&`

Signature:
```angelscript
const CubemapFace& in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
const CubemapFace& in
```

No description given


## ModelNode
C++ type: `CE::Renderer::Resources::Model::Node`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### Transform
Type: `Mat4`

#### mesh_indices
Type: `CE::Containers::Uint32Vector`

#### children
Type: `CE::Containers::Uint32Vector`

### Behaviours
#### Construct
No description given

#### Construct
Signature:
```angelscript
const ModelNode& in
```

No description given

#### Destruct
No description given

### Operators
#### =
Return type: `ModelNode&`

Signature:
```angelscript
const ModelNode& in
```

No description given


## Model
C++ type: `CE::Renderer::Resources::Model`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### meshes
Type: `CE::Containers::MeshVector`

#### materials
Type: `CE::Containers::MaterialVector`

#### mesh_material_indices
Type: `CE::Containers::Uint32Vector`

#### nodes
Type: `CE::Containers::ModelNodeVector`

#### root_node
Type: `uint`

### Behaviours
#### Construct
No description given

#### Construct
Signature:
```angelscript
const Model& in
```

No description given

#### Destruct
No description given

### Methods
#### SetAllMeshMaterials
Return type: `void`

Signature:
```angelscript
const Material& in
```

No description given

### Operators
#### =
Return type: `Model&`

Signature:
```angelscript
const Model& in
```

No description given


## Cubemap
C++ type: `CE::Scripting::Bindings::ASCubemap`

Flags: `Value`

No description given

### Properties
#### right
Type: `CubemapFace`

#### left
Type: `CubemapFace`

#### top
Type: `CubemapFace`

#### bottom
Type: `CubemapFace`

#### front
Type: `CubemapFace`

#### back
Type: `CubemapFace`

### Behaviours
#### Construct
No description given

#### Construct
Signature:
```angelscript
const Texture& in left, const Texture& in right = NoTexture, const Texture& in top = NoTexture, const Texture& in bottom = NoTexture, const Texture& in front = NoTexture, const Texture& in back = NoTexture
```

No description given

#### Destructor
No description given

### Operators
#### =
Return type: `Cubemap&`

Signature:
```angelscript
Cubemap& in
```

No description given


## Camera3D
C++ type: `CE::Renderer::Camera3D`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### position
Type: `Vec3`

#### rotation
Type: `Vec3`

#### target
Type: `Vec3`

#### up
Type: `Vec3`

#### fov
Type: `float`

#### near_clip
Type: `float`

#### far_clip
Type: `float`

#### ortho_size
Type: `float`

#### aspect_override
Type: `float`

#### use_target
Type: `bool`

#### projection_mode
Type: `Camera3DProjectionMode`

### Behaviours
#### Construct
No description given

#### Destruct
No description given


## CE::Graphics::Render3D
## Enums
### Camera3DProjectionMode
C++ type: `CE::Renderer::Camera3D::ProjectionMode`

No description given

Values:

- `Perspective`
- `Orthographic`
