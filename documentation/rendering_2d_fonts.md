# CE::Graphics::Render2D::Text

## Functions
### DrawText
Return type: `void`

Signature:
```angelscript
const string& in text, int x, int y, float size
```

Draws text using the default font, and the colour is black

### DrawText
Return type: `void`

Signature:
```angelscript
const string& in text, int x, int y, float size, Colour& in colour
```

Draws text using the default font

### DrawText
Return type: `void`

Signature:
```angelscript
const string& in text, const string& in name, int x, int y, float size, Colour& in colour
```

Draws text using the font name specified

### LoadFont
Return type: `bool`

Signature:
```angelscript
const string& in path, const string& in name
```

Loads a font from the VFS, returns false if failed, true if succeded

### SetDefaultFont
Return type: `void`

Signature:
```angelscript
const string& in font_name
```

Sets the default font for DrawText to use

### UnloadFont
Return type: `void`

Signature:
```angelscript
const string& in font_name
```

Unloads the specified font

### UnloadAllFonts
Return type: `void`

Unloads all loaded fonts
