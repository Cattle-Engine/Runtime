# CE::Window

## WindowSize
C++ type: `CE::Common::Window::WindowSize`

Flags: `Value`, `POD`, `AutoGetFlags`

No description given

### Properties
#### w
Type: `int`

#### h
Type: `int`


## Enums
### WindowMode
C++ type: `CE::Common::Window::WindowMode`

Values:

- `Fullscreen`
- `Borderless`
- `Windowed`

## Functions
### HideWindow
Return type: `void`

Signature:
```angelscript
bool hidden
```

Hides a window. This is different from minimising a window as it doesn't show a window icon on the taskbar,
as well as not displaying the window

### IsWindowHidden
Return type: `bool`

Returns true/false if the window is hidden or shown

### MinimiseWindow
Return type: `void`

Signature:
```angelscript
bool hidden
```

Minimises a window. Unlike hiding a window this still shows an icon for the window on the taskbar

### IsWindowMinimised
Return type: `bool`

Returns true/false if a window is minimised or shown

### SetWindowMode
Return type: `bool`

Signature:
```angelscript
WindowMode mode
```

Sets the window mode. On Fullscreen and Windowed this will always follow user defined resolutions.
On Borderless windows it uses the desktop resolution and the renderer will use the desktop resolutions.
This will be updated so when SetWindowMode is called it will automatically set the renderer resolution to the
window resolution.

### GetWindowMode
Return type: `WindowMode`

Returns the current window mode

### SetWindowSize
Return type: `bool`

Signature:
```angelscript
WindowSize, WindowMode
```

Sets the window size for a specified window mode

### GetWindowSize
Return type: `WindowSize`

Signature:
```angelscript
WindowMode mode
```

Gets the window size for the specficied window mode

### SetWindowTitle
Return type: `void`

Signature:
```angelscript
string title
```

Sets the window title

### GetWindowTitle
Return type: `string`

Gets the current window title, if no title it returns ""

### SetWindowResizable
Return type: `void`

Signature:
```angelscript
bool resizable
```

Sets the window to be resizable or not resizable

### IsWindowResizable
Return type: `bool`

Returns true/false if the window is resizable or not

### SetWindowIcon
Return type: `bool`

Signature:
```angelscript
string path
```

Sets the window icon
