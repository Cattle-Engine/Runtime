# CE::Graphics::Render3D::Primitives

## Functions
### CreateCube
Return type: `MeshData`

Signature:
```angelscript
const Vec3 size = Vec3(1.0f, 1.0f, 1.0f)
```

Creates a 3D cube mesh

### CreateCube
Return type: `MeshData`

Signature:
```angelscript
const Vec3 size, const Colour colour
```

Creates a 3D cube mesh with colour

### CreatePlane
Return type: `MeshData`

Signature:
```angelscript
const Vec2 size = Vec2(1.0f, 1.0f)
```

Creates a 3D plane mesh

### CreatePlane
Return type: `MeshData`

Signature:
```angelscript
const Vec2 size, const Colour colour
```

Creates a 3D plane mesh with colour

### CreateSphere
Return type: `MeshData`

Signature:
```angelscript
float radius = 0.5f, int segments = 16, int rings = 16
```

Creates a 3D sphere

### CreateSphere
Return type: `MeshData`

Signature:
```angelscript
float radius, int segments, int rings, Colour colour
```

Creates a 3D sphere with colour

### CreateCapsule
Return type: `MeshData`

Signature:
```angelscript
float radius = 0.5f, float height = 1.0f, int segments = 16
```

Creates a 3D capsule mesh

### CreateCapsule
Return type: `MeshData`

Signature:
```angelscript
float radius, float height, int segments, Colour colour
```

Creates a 3D capsule mesh with colour

### CreateCylinder
Return type: `MeshData`

Signature:
```angelscript
float radius = 0.5f, float height = 1.0f, int segments = 16
```

Creates a 3D cylinder mesh

### CreateCylinder
Return type: `MeshData`

Signature:
```angelscript
float radius, float height, int segments, Colour colour
```

Creates a 3D cylinder mesh with colour

### CreateTorus
Return type: `MeshData`

Signature:
```angelscript
float radius = 1.0f, float tubeRadius = 0.25f, int segments = 24, int tubeSegments = 12
```

Creates a 3D torus mesh

### CreateTorus
Return type: `MeshData`

Signature:
```angelscript
float radius, float tubeRadius, int segments, int tubeSegments, Colour colour
```

Creates a 3D torus mesh

### CreateCone
Return type: `MeshData`

Signature:
```angelscript
float radius = 0.5f, float height = 1.0f, int segments = 16
```

Creates a 3D cone mesh

### CreateCone
Return type: `MeshData`

Signature:
```angelscript
float radius, float height, int segments, Colour colour
```

Creates a 3D cone mesh

### SetMeshColour
Return type: `void`

Signature:
```angelscript
MeshData& in mesh, const Colour colour
```

Sets the colour of a mesh
