# CE

Registration priority: `100`

## Vec4
C++ type: `glm::vec4`

Flags: `Value`, `AppClassCDAK`

No description given

### Properties
#### x
Type: `float`

#### y
Type: `float`

#### z
Type: `float`

#### w
Type: `float`

### Behaviours
#### Construct
No description given

#### Construct
Signature:
```angelscript
float, float, float, float
```

No description given

#### Construct
Signature:
```angelscript
const Vec4& in
```

No description given

#### Destruct
No description given

### Methods
#### Length
Return type: `float`

Const method

No description given

#### LengthSquared
Return type: `float`

Const method

No description given

#### Normalize
Return type: `Vec4`

Const method

No description given

#### Dot
Return type: `float`

Signature:
```angelscript
const Vec4& in
```

Const method

No description given

#### Distance
Return type: `float`

Signature:
```angelscript
const Vec4& in
```

Const method

No description given

#### Lerp
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in, float
```

Const method

No description given

#### Clamp
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in, const Vec4& in
```

Const method

No description given

#### Abs
Return type: `Vec4`

Const method

No description given

#### ToString
Return type: `string`

Const method

No description given

#### NormalizeInPlace
Return type: `void`

No description given

#### Set
Return type: `void`

Signature:
```angelscript
float, float, float, float
```

No description given

#### IsZero
Return type: `bool`

Const method

No description given

### Operators
#### =
Return type: `Vec4 &`

Signature:
```angelscript
const Vec4& in
```

No description given

#### +
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in
```

No description given

#### -
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in
```

No description given

#### /
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in
```

No description given

#### *
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in
```

No description given

#### *
Return type: `Vec4`

Signature:
```angelscript
float
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
const Vec4& in
```

No description given


## Colour
C++ type: `CE::Renderer::Colour`

Flags: `Value`, `POD`

No description given

### Properties
#### r
Type: `uint8`

#### g
Type: `uint8`

#### b
Type: `uint8`

#### a
Type: `uint8`

### Behaviours
#### Construct
No description given

#### Construct
Signature:
```angelscript
uint8, uint8, uint8, uint8
```

No description given


## Material
C++ type: `CE::Renderer::Resources::MaterialHandle`

Flags: `Value`, `POD`

No description given

### Properties
#### handle
Type: `uint64`

### Operators
#### ==
Return type: `bool`

Signature:
```angelscript
const Material & in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
int64
```

No description given


## Mesh
C++ type: `CE::Renderer::Resources::MeshHandle`

Flags: `Value`, `POD`

No description given

### Properties
#### handle
Type: `uint64`

### Operators
#### ==
Return type: `bool`

Signature:
```angelscript
const Mesh & in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
int64
```

No description given


## Shader
C++ type: `CE::Renderer::Resources::ShaderHandle`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### handle
Type: `uint64`

### Behaviours
#### Construct
No description given

#### Construct
Signature:
```angelscript
const Shader& in
```

No description given

#### Destruct
No description given

### Operators
#### ==
Return type: `bool`

Signature:
```angelscript
const Shader & in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
int64
```

No description given

#### =
Return type: `Shader`

Signature:
```angelscript
Shader & in
```

No description given


## Texture
C++ type: `CE::Renderer::Resources::TextureHandle`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### handle
Type: `uint64`

### Behaviours
#### Construct
No description given

#### Construct
Signature:
```angelscript
const Texture& in
```

No description given

#### Destruct
No description given

### Operators
#### ==
Return type: `bool`

Signature:
```angelscript
const Texture & in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
int64
```

No description given

#### =
Return type: `Texture`

Signature:
```angelscript
Texture & in
```

No description given


## Vec2
C++ type: `glm::vec2`

Flags: `Value`, `AppClassCDAK`

No description given

### Properties
#### x
Type: `float`

#### y
Type: `float`

### Behaviours
#### Construct
No description given

#### Construct
Signature:
```angelscript
float, float
```

No description given

#### Construct
Signature:
```angelscript
const Vec2& in
```

No description given

#### Destruct
No description given

### Methods
#### Length
Return type: `float`

Const method

No description given

#### LengthSquared
Return type: `float`

Const method

No description given

#### Normalize
Return type: `Vec2`

Const method

No description given

#### Dot
Return type: `float`

Signature:
```angelscript
const Vec2 & in
```

Const method

No description given

#### Distance
Return type: `float`

Signature:
```angelscript
const Vec2 & in
```

Const method

No description given

#### Lerp
Return type: `Vec2`

Signature:
```angelscript
const Vec2 & in, float
```

Const method

No description given

#### Reflect
Return type: `Vec2`

Signature:
```angelscript
const Vec2 & in
```

Const method

No description given

#### Clamp
Return type: `Vec2`

Signature:
```angelscript
const Vec2 & in, const Vec2 & in
```

Const method

No description given

#### Abs
Return type: `Vec2`

Const method

No description given

#### ToString
Return type: `string`

Const method

No description given

#### NormalizeInPlace
Return type: `void`

No description given

#### Set
Return type: `void`

Signature:
```angelscript
float, float
```

No description given

#### IsZero
Return type: `bool`

Const method

No description given

#### Angle
Return type: `float`

Const method

No description given

#### Rotate
Return type: `Vec2`

Signature:
```angelscript
float
```

Const method

No description given

### Operators
#### =
Return type: `Vec2 &`

Signature:
```angelscript
const Vec2& in
```

No description given

#### +
Return type: `Vec2`

Signature:
```angelscript
const Vec2& in
```

No description given

#### -
Return type: `Vec2`

Signature:
```angelscript
const Vec2& in
```

No description given

#### /
Return type: `Vec2`

Signature:
```angelscript
const Vec2& in
```

No description given

#### *
Return type: `Vec2`

Signature:
```angelscript
const Vec2& in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
const Vec2 & in
```

No description given


## Vec3
C++ type: `glm::vec3`

Flags: `Value`, `AppClassCDAK`

No description given

### Properties
#### x
Type: `float`

#### y
Type: `float`

#### z
Type: `float`

### Behaviours
#### Construct
No description given

#### Construct
Signature:
```angelscript
float, float, float
```

No description given

#### Construct
Signature:
```angelscript
const Vec3& in
```

No description given

#### Destruct
No description given

### Methods
#### Length
Return type: `float`

Const method

No description given

#### LengthSquared
Return type: `float`

Const method

No description given

#### Normalize
Return type: `Vec3`

Const method

No description given

#### Dot
Return type: `float`

Signature:
```angelscript
const Vec3& in
```

Const method

No description given

#### Cross
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```

Const method

No description given

#### Distance
Return type: `float`

Signature:
```angelscript
const Vec3& in
```

Const method

No description given

#### Lerp
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in, float
```

Const method

No description given

#### Reflect
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```

Const method

No description given

#### Refract
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in, float
```

Const method

No description given

#### Clamp
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in, const Vec3& in
```

Const method

No description given

#### Abs
Return type: `Vec3`

Const method

No description given

#### ToString
Return type: `string`

Const method

No description given

#### NormalizeInPlace
Return type: `void`

No description given

#### Set
Return type: `void`

Signature:
```angelscript
float, float, float
```

No description given

#### IsZero
Return type: `bool`

Const method

No description given

#### Rotate
Return type: `Vec3`

Signature:
```angelscript
float, const Vec3& in
```

Const method

No description given

### Operators
#### =
Return type: `Vec3&`

Signature:
```angelscript
const Vec3& in
```

No description given

#### +
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```

No description given

#### -
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```

No description given

#### /
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```

No description given

#### *
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
const Vec3& in
```

No description given


## Vertex3D
C++ type: `CE::Renderer::Vertex3D`

Flags: `Value`

No description given

### Properties
#### position
Type: `Vec3`

#### normal
Type: `Vec3`

#### colour
Type: `Colour`

#### uv
Type: `Vec2`

#### tangent
Type: `Vec3`

#### tangent_sign
Type: `float`

### Behaviours
#### Construct
No description given

#### Destructor
No description given


## MeshData
C++ type: `CE::Renderer::MeshData`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### vertices
Type: `CE::Containers::Vertex3DVector`

#### indices
Type: `CE::Containers::Uint32Vector`

### Behaviours
#### Construct
No description given

#### Destructor
No description given

### Operators
#### =
Return type: `MeshData&`

Signature:
```angelscript
MeshData& in other
```

No description given


## Transform3D
C++ type: `CE::Renderer::Transform3D`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### position
Type: `Vec3`

#### rotation
Type: `Vec3`

#### scale
Type: `Vec3`

### Behaviours
#### Constructor
No description given

#### Destructor
No description given

### Operators
#### =
Return type: `Transform3D&`

Signature:
```angelscript
Transform3D& in other
```

No description given


## Mat4
C++ type: `glm::mat4`

Flags: `Value`, `AppClassCDAK`

No description given

### Behaviours
#### Construct
No description given

#### Construct
Signature:
```angelscript
float
```

No description given

#### Construct
Signature:
```angelscript
const Mat4& in
```

No description given

#### Destruct
No description given

### Methods
#### Transpose
Return type: `Mat4`

Const method

No description given

#### Inverse
Return type: `Mat4`

Const method

No description given

#### Determinant
Return type: `float`

Const method

No description given

#### ToString
Return type: `string`

Const method

No description given

#### SetIdentity
Return type: `void`

No description given

#### Translate
Return type: `void`

Signature:
```angelscript
const Vec3& in
```

No description given

#### Scale
Return type: `void`

Signature:
```angelscript
const Vec3& in
```

No description given

#### Rotate
Return type: `void`

Signature:
```angelscript
float, const Vec3& in
```

No description given

### Operators
#### =
Return type: `Mat4&`

Signature:
```angelscript
const Mat4& in
```

No description given

#### +
Return type: `Mat4`

Signature:
```angelscript
const Mat4& in
```

No description given

#### -
Return type: `Mat4`

Signature:
```angelscript
const Mat4& in
```

No description given

#### *
Return type: `Mat4`

Signature:
```angelscript
const Mat4& in
```

No description given

#### *
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
const Mat4& in
```

No description given


## CE
## Enums
### ShaderStage
C++ type: `CE::Renderer::ShaderStage`

No description given

Values:

- `Vertex`
- `Fragment`

### TextureFlip
C++ type: `CE::Renderer::TextureFlip`

No description given

Values:

- `None`
- `Horizontal`
- `Vertical`

### TextureFilter
C++ type: `CE::Renderer::TextureFilter`

No description given

Values:

- `Nearest`
- `Linear`

## Constants
### NoTexture
Type: `Texture`

Value: `0`

No description given
