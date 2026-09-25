# Description
Minimal immediate-mode UI bindings. Call these only from the optional void imgui()
script function, which runs after the engine starts an ImGui frame and before it renders.

# CE::ImGui

## Functions
### Begin
Return type: `bool`

Signature:
```angelscript
const string& in title
```

Begins a window and returns whether its contents should be drawn; always pair it with End.

### End
Return type: `void`

Ends the current window begun by Begin.

### Text
Return type: `void`

Signature:
```angelscript
const string& in text
```

Draws unformatted text at the current cursor position.

### Button
Return type: `bool`

Signature:
```angelscript
const string& in label
```

Draws a button and returns true on the frame it is clicked.

### Separator
Return type: `void`

Draws a horizontal separator.

### SameLine
Return type: `void`

Places the next item on the current line.
