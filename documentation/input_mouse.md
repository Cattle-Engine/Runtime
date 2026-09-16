# CE::Input::Mouse

## Enums
### MouseButtons
C++ type: `CE::Input::MouseButtons`

Values:

- `Left`
- `Middle`
- `Right`
- `X1`
- `X2`

### MouseVisiblity
C++ type: `CE::Input::MouseVisibility`

Values:

- `Hidden`
- `Shown`

## Functions
### GetX
Return type: `int`

Gets the X position of the mouse pointer

### GetY
Return type: `int`

Gets the Y position of the mouse pointer

### GetDeltaX
Return type: `int`

Gets how much the mouse pointer has moved since the last frame, on the X axis

### GetDeltaY
Return type: `int`

Gets how much the mousep ointer has moved since the last frame, on the Y axis

### GetWheelX
Return type: `int`

Gets how much the mouse wheel has moved since the last frame, on the X axis (up/down)

### GetWheelY
Return type: `int`

Gets how much the mouse wheel has moved since the last frame on the Y axis (left/right).
This is really a feature of trackpads and not mouse wheels, or trackballs ig.

### SetCursorVisibilty
Return type: `void`

Signature:
```angelscript
MouseVisiblity visiblity
```

Shows or hides the cursor

### LockCursor
Return type: `void`

Signature:
```angelscript
bool lock
```

Locks the cursor to the center of the screen if true

### IsButtonDown
Return type: `bool`

Signature:
```angelscript
MouseButtons button
```

Checks if a mouse button is down

### IsButtonPressed
Return type: `bool`

Signature:
```angelscript
MouseButtons button
```

Checks if a mouse button is pressed

### IsButtonReleased
Return type: `bool`

Signature:
```angelscript
MouseButtons
```

Checks if a mouse button is not pressed
