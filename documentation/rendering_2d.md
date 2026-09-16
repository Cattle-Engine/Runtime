# CE::Graphics::Render2D

## Functions
### DrawTexture
Return type: `void`

Signature:
```angelscript
const Texture& in, float x, float y, float w, float h, const Colour& in tint, float rotation, TextureFlip flip = TextureFlip::None
```

No description given

### DrawTexture
Return type: `void`

Signature:
```angelscript
const Texture& in, float x, float y, const Colour& in, float rotation, TextureFlip flip = TextureFlip::None
```

No description given

### DrawTexture
Return type: `void`

Signature:
```angelscript
const Texture& in, float x, float y, const Colour& in
```

No description given

### DrawTexture
Return type: `void`

Signature:
```angelscript
const Texture& in, float x, float y
```

No description given

### DrawTextureUV
Return type: `void`

Signature:
```angelscript
const Texture& in, float x, float y, float u0, float v0, float u1, float v1, const Colour& in tint, float rotation, TextureFlip flip = TextureFlip::None
```

No description given

### DrawTextureUV
Return type: `void`

Signature:
```angelscript
const Texture& in, float x, float y, float u0, float v0, float u1, float v1, const Colour& in tint, float rotation
```

No description given

### DrawTextureUV
Return type: `void`

Signature:
```angelscript
const Texture& in, float x, float y, float u0, float v0, float u1, float v1, const Colour& in tint
```

No description given

### DrawTextureUV
Return type: `void`

Signature:
```angelscript
const Texture& in, float x, float y, float u0, float v0, float u1, float v1
```

Draws a 2D texture

### DrawTriangle
Return type: `void`

Signature:
```angelscript
float x0, float y0, float x1, float y1, float x2, float y2, const Colour& in, float rotation
```

Draws a 2D triangle

### DrawRectangleLines
Return type: `void`

Signature:
```angelscript
float x, float y, float w, float h, float thickness, const Colour& in
```

Draw a 2D rectangle made out of lines

### DrawRectangle
Return type: `void`

Signature:
```angelscript
float x, float y, float w, float h, const Colour& in, float rotation
```

Draw a filled 2D rectangle

### DrawCircle
Return type: `void`

Signature:
```angelscript
float x, float y, float radius, int segments, const Colour& in
```

Draw a filled 2D circle

### DrawCircleLines
Return type: `void`

Signature:
```angelscript
float x, float y, float radius, int segments, float thickness, const Colour& in
```

Draw a 2D circle out of lines

### DrawLine
Return type: `void`

Signature:
```angelscript
float x1, float y1, float x2, float y2, float thickness, const Colour& in
```

Draw a 2D line

### Set2DCameraPos
Return type: `void`

Signature:
```angelscript
Camera2D& in camera
```

Sets the 2D camera position
