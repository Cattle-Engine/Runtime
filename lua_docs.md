
# API Documentation

You can use Color like this:
```lua
local RED = Color(255, 0, 0)
```

## "CE.*" 

### ```GetGameName()```
Returns the game name as a string

### ```GetGameVersion()```
Returns the game version as a string

### ```IsDebugOn()```
Returns true if debug is on

### ```Shutdown()```
Shuts down the game

### ```SetGameState(name)```
Sets the current game state string (used by callbacks)

### ```GetGameState()```
Returns the current game state string

## "Mouse.*"

### ```Hide()```
Hides the cursor if not already hidden

### ```Show()```
Shows the cursor if not already hidden

### ``Enable()``
Enables the cursor (Unlocks it)

### ```Disable()```
Disables the cursor (locks it)

### ```IsCursorHidden()```
Returns true if hidden, false if not

### ```IsCursorOnScreen()```
Returns true if the cursor is on the window

### ```GetMousePosition```
Used like this ```local x, y = Mouse.GetMousePosition()```

## "Input.*"

### Keyboard key list:

#### Letters 
A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
#### Numbers: 
0, 1, 2, 3, 4, 5, 6, 7, 8, 9
#### Function keys: 
F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24
#### NumPad keys: 
KP0, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9
KP_DECIMAL, KP_DIVIDE, KP_MULTIPLY, KP_SUBTRACT, KP_ADD, KP_ENTER, KP_EQUAL
#### Special keys:
SPACE, ENTER, RETURN, TAB, BACKSPACE, ESC, ESCAPE
LEFT, RIGHT, UP, DOWN
HOME, END, PAGE_UP, PAGE_DOWN
INSERT, DELETE
SHIFT, LEFT_SHIFT, RIGHT_SHIFT
CTRL, CONTROL, LEFT_CONTROL, RIGHT_CONTROL
ALT, LEFT_ALT, RIGHT_ALT
SUPER, LEFT_SUPER, RIGHT_SUPER
CAPS_LOCK, NUM_LOCK, SCROLL_LOCK
PAUSE, PRINT_SCREEN, MENU
APOSTROPHE, QUOTE, COMMA, PERIOD, DOT, SLASH, SEMICOLON
EQUAL, MINUS, LEFT_BRACKET, RIGHT_BRACKET, BACKSLASH
GRAVE, BACKTICK

### "IsKeyPressed("A")"
Returns true if pressed once

### "IsKeyPressedRepeat("A")"
Returns true if a key has been pressed again

### "IsKeyDown("A")"
Returns true if a key has been pressed

### "IsKeyReleased("A")"
Returns true if released once

### "IsKeyUp("A")"
Returns true if a key is not being pressed

## "Misc.*"

### "OpenUrl("https://example.com")
Returns nothing

## "Time.*"

### "GetUptime()"
Returns the elapsed since startup as an int

### "GetDeltaTime()"
Returns time since the last frames as a float

### "GetOSTime()"
Returns the OS time

## "Fonts.*"

### "Load("Main", "fonts/main.ttf")
Loads a font

### "LoadEx("Main", 2, "fonts/main.ttf")
Load a font, arg2 is the size of the font

### "Unload("Main")"
Unload a font

### "UnloadAll()"
Unload all fonts

### "SetDefault("Main")"
Set the default font for Text.Draw


## "Text.*"

### "Draw("Something", 1, 100, 100, Color(0, 0, 0))"
Arg1 is the text you want to draw, arg2 is size, Arg3 and 4 are Pos X and Y
Color is optional, it defaults to black

### "DrawEx("Main" "Something", 1, 100, 100, Color(0, 0, 0))
Arg1 is a loaded font name, arg2 is the text you want to draw, arg3 is size, Arg4 and 5 are Pos X and Y.
Color is optional, it defaults to black

## "Textures.*"

### "Load("Player", "textures/player.png")
Arg1 is name for the texture, Arg2 is the path

### "LoadFolder("textures")"
Loads a folder of textures. If Eg you loaded the folder "textures" the name would look like "textures/foo"

### "Unload("Namething")"
Unload a single texture

### "UnloadAll()"
Unload all textures

### "Draw("Player", 100, 100, Color(255, 255, 255, 0))"
Draw a texture, Color is optional, defaults to WHITE

### "Exists("Player")
Returns true if the texture exists

### "Loaded()"
Returns an int of all loaded textures (including errors)

### "LoadedNoError()"
Returns an int of all loaded textures (not including errors)

### "LoadedError()"
Returns an int of all loaded textures (only errors)

## "Audio.*"

### "UnloadEverything()"
Unload EVERYTHING, including wav, music and sfx

### "LoadFolder("sfxs")
Load a folder, this is loads sfx not music

### "SFX.*"

#### "Load("explosion", "sfx/explosion.mp3)
Load an SFX

#### "Play("explosion")
Play an SFX

#### "Unload("explosion")"
Unload an SFX

#### "UnloadAll()"
Unload all SFX's

### "Wave.*"

#### "Load("explosion", "sfx/explosion.wav)
Load a wave

#### "Play("explosion")
Play a wave

#### "Unload("explosion")"
Unload a wave

#### "UnloadAll()"
Unload all wave's

### "Music.*"

#### "Load("bgm", "sfx/bgm.mp3)
Load a music stream

#### "LoadFolder("music")
Load a folder of music

#### "Play("bgm")
Play a music stream

#### "Pause("bgm")
Pause a music stream

#### "Resume("bgm")
Resume a music stream

## "Callbacks.*"

Callbacks.On(state, event, fn)	Registers a callback fn for a given state and event. state can be "*" to match all states.
Callbacks.Once(state, event, fn)	Same as On but the callback runs only once.
Callbacks.Off(id)	Unregisters a callback by its ID.
Callbacks.Clear()	Clears all callbacks.
Callbacks.Emit(event, dt?)	Manually emit an event. Optionally pass a number dt (like delta time).
Callbacks.GetState()	Returns the current state name as a string.
Callbacks.SetState(state)	Changes the current state.
Callbacks.OnUpdate(fn)	Shortcut for On("*", "Update", fn)
Callbacks.OnceUpdate(fn)	Shortcut for Once("*", "Update", fn)
Similarly: OnDraw, OnEnter, OnExit, OnceDraw, etc.	

## "Draw."

### "Clear(Color(0, 0, 0))
This clears the background and sets it to black

### "Rect(50, 50, 200, 100, Color(0, 0, 0))"
This draws a rectangle at X 50, Y 50, Width 200, Height 100. 
Default Color is WHITE

### "Circle(300, 300, 50, Color(255, 0, 0))
This draws a circle at 300, 300, with a radius of 50.
Default Color is WHITE

### "Line(10, 20, 100, 200, Color(255,0,0))"
The first 2 args is start posX and PosY, the next 2 is end PosX and PosY.
Default Color is WHITE


## "Data.*"

### "GetFile("foo.txt")
Tries to open a file in the game data dir, returns a string.
