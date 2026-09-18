# CE

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

#### Construct
Signature:
```angelscript
float, float, float, float
```


#### Construct
Signature:
```angelscript
const Vec4& in
```


#### Destruct

### Methods
#### Length
Return type: `float`

Const method


#### LengthSquared
Return type: `float`

Const method


#### Normalize
Return type: `Vec4`

Const method


#### Dot
Return type: `float`

Signature:
```angelscript
const Vec4& in
```

Const method


#### Distance
Return type: `float`

Signature:
```angelscript
const Vec4& in
```

Const method


#### Lerp
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in, float
```

Const method


#### Clamp
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in, const Vec4& in
```

Const method


#### Abs
Return type: `Vec4`

Const method


#### ToString
Return type: `string`

Const method


#### NormalizeInPlace
Return type: `void`


#### Set
Return type: `void`

Signature:
```angelscript
float, float, float, float
```


#### IsZero
Return type: `bool`

Const method


### Operators
#### =
Return type: `Vec4 &`

Signature:
```angelscript
const Vec4& in
```


#### +
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in
```


#### -
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in
```


#### /
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in
```


#### *
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in
```


#### *
Return type: `Vec4`

Signature:
```angelscript
float
```


#### ==
Return type: `bool`

Signature:
```angelscript
const Vec4& in
```



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

#### Construct
Signature:
```angelscript
uint8, uint8, uint8, uint8
```



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


#### ==
Return type: `bool`

Signature:
```angelscript
int64
```



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


#### ==
Return type: `bool`

Signature:
```angelscript
int64
```



## Shader
C++ type: `CE::Renderer::Resources::ShaderHandle`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### handle
Type: `uint64`

### Behaviours
#### Construct

#### Construct
Signature:
```angelscript
const Shader& in
```


#### Destruct

### Operators
#### ==
Return type: `bool`

Signature:
```angelscript
const Shader & in
```


#### ==
Return type: `bool`

Signature:
```angelscript
int64
```



## Texture
C++ type: `CE::Renderer::Resources::TextureHandle`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### handle
Type: `uint64`

### Behaviours
#### Construct

#### Construct
Signature:
```angelscript
const Texture& in
```


#### Destruct

### Operators
#### ==
Return type: `bool`

Signature:
```angelscript
const Texture & in
```


#### ==
Return type: `bool`

Signature:
```angelscript
int64
```


#### =
Return type: `Texture`

Signature:
```angelscript
Texture & in
```



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

#### Construct
Signature:
```angelscript
float, float
```


#### Construct
Signature:
```angelscript
const Vec2& in
```


#### Destruct

### Methods
#### Length
Return type: `float`

Const method


#### LengthSquared
Return type: `float`

Const method


#### Normalize
Return type: `Vec2`

Const method


#### Dot
Return type: `float`

Signature:
```angelscript
const Vec2 & in
```

Const method


#### Distance
Return type: `float`

Signature:
```angelscript
const Vec2 & in
```

Const method


#### Lerp
Return type: `Vec2`

Signature:
```angelscript
const Vec2 & in, float
```

Const method


#### Reflect
Return type: `Vec2`

Signature:
```angelscript
const Vec2 & in
```

Const method


#### Clamp
Return type: `Vec2`

Signature:
```angelscript
const Vec2 & in, const Vec2 & in
```

Const method


#### Abs
Return type: `Vec2`

Const method


#### ToString
Return type: `string`

Const method


#### NormalizeInPlace
Return type: `void`


#### Set
Return type: `void`

Signature:
```angelscript
float, float
```


#### IsZero
Return type: `bool`

Const method


#### Angle
Return type: `float`

Const method


#### Rotate
Return type: `Vec2`

Signature:
```angelscript
float
```

Const method


### Operators
#### =
Return type: `Vec2 &`

Signature:
```angelscript
const Vec2& in
```


#### +
Return type: `Vec2`

Signature:
```angelscript
const Vec2& in
```


#### -
Return type: `Vec2`

Signature:
```angelscript
const Vec2& in
```


#### /
Return type: `Vec2`

Signature:
```angelscript
const Vec2& in
```


#### *
Return type: `Vec2`

Signature:
```angelscript
const Vec2& in
```


#### ==
Return type: `bool`

Signature:
```angelscript
const Vec2 & in
```



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

#### Construct
Signature:
```angelscript
float, float, float
```


#### Construct
Signature:
```angelscript
const Vec3& in
```


#### Destruct

### Methods
#### Length
Return type: `float`

Const method


#### LengthSquared
Return type: `float`

Const method


#### Normalize
Return type: `Vec3`

Const method


#### Dot
Return type: `float`

Signature:
```angelscript
const Vec3& in
```

Const method


#### Cross
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```

Const method


#### Distance
Return type: `float`

Signature:
```angelscript
const Vec3& in
```

Const method


#### Lerp
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in, float
```

Const method


#### Reflect
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```

Const method


#### Refract
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in, float
```

Const method


#### Clamp
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in, const Vec3& in
```

Const method


#### Abs
Return type: `Vec3`

Const method


#### ToString
Return type: `string`

Const method


#### NormalizeInPlace
Return type: `void`


#### Set
Return type: `void`

Signature:
```angelscript
float, float, float
```


#### IsZero
Return type: `bool`

Const method


#### Rotate
Return type: `Vec3`

Signature:
```angelscript
float, const Vec3& in
```

Const method


### Operators
#### =
Return type: `Vec3&`

Signature:
```angelscript
const Vec3& in
```


#### +
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```


#### -
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```


#### /
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```


#### *
Return type: `Vec3`

Signature:
```angelscript
const Vec3& in
```


#### ==
Return type: `bool`

Signature:
```angelscript
const Vec3& in
```



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

#### Destructor


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

#### Destructor

### Operators
#### =
Return type: `MeshData&`

Signature:
```angelscript
MeshData& in other
```



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

#### Destructor

### Operators
#### =
Return type: `Transform3D&`

Signature:
```angelscript
Transform3D& in other
```



## Mat4
C++ type: `glm::mat4`

Flags: `Value`, `AppClassCDAK`

No description given

### Behaviours
#### Construct

#### Construct
Signature:
```angelscript
float
```


#### Construct
Signature:
```angelscript
const Mat4& in
```


#### Destruct

### Methods
#### Transpose
Return type: `Mat4`

Const method


#### Inverse
Return type: `Mat4`

Const method


#### Determinant
Return type: `float`

Const method


#### ToString
Return type: `string`

Const method


#### SetIdentity
Return type: `void`


#### Translate
Return type: `void`

Signature:
```angelscript
const Vec3& in
```


#### Scale
Return type: `void`

Signature:
```angelscript
const Vec3& in
```


#### Rotate
Return type: `void`

Signature:
```angelscript
float, const Vec3& in
```


### Operators
#### =
Return type: `Mat4&`

Signature:
```angelscript
const Mat4& in
```


#### +
Return type: `Mat4`

Signature:
```angelscript
const Mat4& in
```


#### -
Return type: `Mat4`

Signature:
```angelscript
const Mat4& in
```


#### *
Return type: `Mat4`

Signature:
```angelscript
const Mat4& in
```


#### *
Return type: `Vec4`

Signature:
```angelscript
const Vec4& in
```


#### ==
Return type: `bool`

Signature:
```angelscript
const Mat4& in
```



## Enums
### ShaderStage
C++ type: `CE::Renderer::ShaderStage`

Values:

- `Fragment`
- `Vertex`

### TextureFlip
C++ type: `CE::Renderer::TextureFlip`

Values:

- `None`
- `Horizontal`
- `Vertical`

## Constants
### NoTexture
Type: `Texture`

Value: `0`

No description given
