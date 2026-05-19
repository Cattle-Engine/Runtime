# API Documentation

You can use Color like this:
```angelscript
CE::Graphics::Colour colour = CE::Graphics::Colour(255, 0, 0, 255);
```

## CE Namespace

### CE.Exit()
Exits the game application

### CE.GetDeltaTime()
Returns the time in seconds since the last frame as a float

### CE.GetFrameTime()
Returns the time in seconds since the last frame as a float (alias for GetDeltaTime)

### CE.GetFPS()
Returns the current frames per second as an integer

### CE.GetInstanceID()
Returns the unique instance ID as an integer

## CE::Settings

### CE.Settings.ReloadSettings()
Reloads the settings from disk

### CE.Settings.GetSettingInt(key, section, fallback)
Gets an integer setting value

### CE.Settings.GetSettingFloat(key, section, fallback)
Gets a float setting value

### CE.Settings.GetSettingBool(key, section, fallback)
Gets a boolean setting value

### CE.Settings.GetSettingString(key, section, fallback)
Gets a string setting value

### CE.Settings.SetSettingInt(key, section, value)
Sets an integer setting value

### CE.Settings.SetSettingFloat(key, section, value)
Sets a float setting value

### CE.Settings.SetSettingBool(key, section, value)
Sets a boolean setting value

### CE.Settings.SetSettingString(key, section, value)
Sets a string setting value

## CE::State

### CE.State.Set(state)
Sets the current game state string

### CE.State.Get()
Returns the current game state string as a string

## CE::Events

### CE.Events.On(state, eventName, callback)
Registers a callback function for a given state and event. State can be "*" to match all states. Returns the callback ID.

### CE.Events.Off(id)
Unregisters a callback by its ID

### CE.Events.Clear()
Clears all registered callbacks

### CE.Events.Emit(event, dt?)
Emits an event manually. Optionally pass a number dt (delta time)

### CE.Events.OnUpdate(callback)
Shortcut for On("*", "Update", callback)

### CE.Events.OnDraw(callback)
Shortcut for On("*", "Draw", callback)

### CE.Events.OnEnter(callback)
Shortcut for On("*", "Enter", callback)

### CE.Events.OnExit(callback)
Shortcut for On("*", "Exit", callback)

### CE.Events.Once(state, eventName, callback)
Same as On but the callback runs only once. Returns the callback ID.

### CE.Events.OnceUpdate(callback)
Shortcut for Once("*", "Update", callback)

### CE.Events.OnceDraw(callback)
Shortcut for Once("*", "Draw", callback)

### CE.Events.OnceEnter(callback)
Shortcut for Once("*", "Enter", callback)

### CE.Events.OnceExit(callback)
Shortcut for Once("*", "Exit", callback)

## CE::Graphics::Textures

### CE.Graphics.Textures.LoadTexture(path, name)
Loads a texture from a file path with the given name

### CE.Graphics.Textures.UnloadTexture(name)
Unloads a texture by name

### CE.Graphics.Textures.DrawTexture(name, x, y, flipX = false, flipY = false, tileX = 1.0, tileY = 1.0)
Draws a texture at position (x, y)

### CE.Graphics.Textures.DrawTextureEx(name, x, y, colour, flipX = false, flipY = false, tileX = 1.0, tileY = 1.0)
Draws a texture with color tint at position (x, y)

### CE.Graphics.Textures.DrawTextureRot(name, x, y, rotation, flipX = false, flipY = false, tileX = 1.0, tileY = 1.0)
Draws a texture with rotation at position (x, y)

### CE.Graphics.Textures.DrawTextureRotEx(name, x, y, rotation, colour, flipX = false, flipY = false, tileX = 1.0, tileY = 1.0)
Draws a texture with rotation and color tint at position (x, y)

### CE.Graphics.Textures.DrawTexturePro(name, x, y, w, h, rotation, colour, flipX = false, flipY = false, tileX = 1.0, tileY = 1.0)
Draws a texture with proportional scaling, rotation, and color

## CE::Graphics::Primitives

### CE.Graphics.Primitives.DrawRectangle(x, y, w, h, colour, rotation = 0.0)
Draws a rectangle at position (x, y) with width w, height h and color

### CE.Graphics.Primitives.DrawCircle(x, y, radius, segments, colour)
Draws a circle at position (x, y) with the given radius and color

### CE.Graphics.Primitives.DrawLine(x1, y1, x2, y2, thickness, colour)
Draws a line from (x1, y1) to (x2, y2) with the given thickness and color

### CE.Graphics.Primitives.DrawTriangle(x0, y0, x1, y1, x2, y2, colour, rotation = 0.0)
Draws a triangle with vertices at (x0, y0), (x1, y1), (x2, y2) and color

### CE.Graphics.Primitives.DrawRectangleLines(x, y, w, h, thickness, colour)
Draws a rectangle outline with the given thickness and color

### CE.Graphics.Primitives.DrawCircleLines(x, y, radius, segments, thickness, colour)
Draws a circle outline with the given thickness and color

## CE::Graphics::Text

### CE.Graphics.Text.LoadFont(path, name, size)
Loads a font from a file path with the given name and size. Returns true if successful

### CE.Graphics.Text.UnloadFont(name)
Unloads a font by name

### CE.Graphics.Text.DrawText(text, x, y, size)
Draws text at position (x, y) with the given size

### CE.Graphics.Text.DrawTextCol(text, x, y, size, colour)
Draws text with color at position (x, y) with the given size

### CE.Graphics.Text.DrawTextEx(text, name, x, y, size, colour)
Draws text using a loaded font by name at position (x, y) with the given size and color

## CE::Graphics::Animations

### CE.Graphics.Animations.LoadAnimation(path, name)
Loads an animation from a file path with the given name

### CE.Graphics.Animations.UnloadAnimation(name)
Unloads an animation by name

### CE.Graphics.Animations.CreateInstance(name)
Creates an instance of an animation. Returns the handle

### CE.Graphics.Animations.DeleteInstance(handle)
Deletes an animation instance by handle

### CE.Graphics.Animations.Play(handle, x, y, loop = false, autoRender = true)
Plays an animation at position (x, y)

### CE.Graphics.Animations.PlayRot(handle, x, y, loop, rotation, autoRender = true)
Plays an animation with rotation at position (x, y)

### CE.Graphics.Animations.SetPosition(handle, x, y, rotation = 0.0)
Sets the position of an animation instance

### CE.Graphics.Animations.Seek(handle, frame)
Seeks to a specific frame in an animation

### CE.Graphics.Animations.SetDrawMode(handle, autoRender)
Sets the draw mode for an animation instance

### CE.Graphics.Animations.SetLooping(handle, loop)
Sets whether an animation should loop

### CE.Graphics.Animations.SetTint(handle, colour)
Sets the color tint for an animation instance

### CE.Graphics.Animations.Pause(handle)
Pauses an animation instance

### CE.Graphics.Animations.Stop(handle)
Stops an animation instance

### CE.Graphics.Animations.DrawFrame(handle)
Draws a single frame of an animation

## CE::Input::KeyboardKeys (Enum)

Keys: KEY_APOSTROPHE, KEY_COMMA, KEY_MINUS, KEY_PERIOD, KEY_SLASH, KEY_ZERO-KEY_NINE, KEY_SEMICOLON, KEY_EQUAL, KEY_A-KEY_Z, KEY_LEFT_BRACKET, KEY_BACKSLASH, KEY_RIGHT_BRACKET, KEY_GRAVE, KEY_SPACE, KEY_ESCAPE, KEY_ENTER, KEY_TAB, KEY_BACKSPACE, KEY_INSERT, KEY_DELETE, KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UP, KEY_PAGE_UP, KEY_PAGE_DOWN, KEY_HOME, KEY_END, KEY_CAPS_LOCK, KEY_SCROLL_LOCK, KEY_NUM_LOCK, KEY_PRINT_SCREEN, KEY_PAUSE, KEY_F1-KEY_F24, KEY_LEFT_SHIFT, KEY_LEFT_CONTROL, KEY_LEFT_ALT, KEY_LEFT_SUPER, KEY_RIGHT_SHIFT, KEY_RIGHT_CONTROL, KEY_RIGHT_ALT, KEY_RIGHT_SUPER, KEY_KB_MENU, KEY_KP_0-KEY_KP_9, KEY_KP_DECIMAL, KEY_KP_DIVIDE, KEY_KP_MULTIPLY, KEY_KP_SUBTRACT, KEY_KP_ADD, KEY_KP_ENTER, KEY_KP_EQUAL

### CE.Input.IsKeyDown(key)
Returns true if a key is currently held down

### CE.Input.IsKeyPressed(key)
Returns true if a key was pressed in the current frame

### CE.Input.IsKeyReleased(key)
Returns true if a key was released in the current frame

## CE::Input::MouseButtons (Enum)

Buttons: LEFT, MIDDLE, RIGHT, X1, X2

### CE.Input.IsMouseButtonDown(button)
Returns true if a mouse button is currently held down

### CE.Input.IsMouseButtonPressed(button)
Returns true if a mouse button was pressed in the current frame

### CE.Input.IsMouseButtonReleased(button)
Returns true if a mouse button was released in the current frame

### CE.Input.GetMouseX()
Returns the current mouse X position as an integer

### CE.Input.GetMouseY()
Returns the current mouse Y position as an integer

### CE.Input.GetMouseDeltaX()
Returns the mouse X delta since last frame as an integer

### CE.Input.GetMouseDeltaY()
Returns the mouse Y delta since last frame as an integer

### CE.Input.GetMouseWheelX()
Returns the mouse wheel X delta as an integer

### CE.Input.GetMouseWheelY()
Returns the mouse wheel Y delta as an integer

## CE::Audio::AudioType (Enum)

Types: SFX, Music

## CE::Audio::EffectType (Enum)

Types: LowPass, HighPass, Reverb, Delay, Chorus

## CE::Audio::AudioEffect

Properties: enabled (bool), type (EffectType), cutoffHz (float), wetMix (float), feedback (float), delayMs (float), depthMs (float), rateHz (float), roomSize (float), damping (float)

### CE.Audio.LoadSound(path, name, type)
Loads a sound from a file path with the given name and type (SFX or Music)

### CE.Audio.UnloadSound(name)
Unloads a sound by name

### CE.Audio.CreateInstance(name)
Creates an instance of a sound. Returns the handle

### CE.Audio.DeleteInstance(handle)
Deletes a sound instance by handle

### CE.Audio.Play(handle)
Plays a sound instance

### CE.Audio.Pause(handle)
Pauses a sound instance

### CE.Audio.Resume(handle)
Resumes a paused sound instance

### CE.Audio.Stop(handle)
Stops a sound instance

### CE.Audio.Seek(handle, seconds)
Seeks to a specific time in a sound instance

### CE.Audio.SetBus(handle, bus)
Sets the audio bus for a sound instance

### CE.Audio.GetBus(handle)
Gets the audio bus for a sound instance

### CE.Audio.SetVolume(handle, volume)
Sets the volume for a sound instance

### CE.Audio.SetMasterVolume(volume)
Sets the master volume (0.0 to 1.0)

### CE.Audio.SetMusicVolume(volume)
Sets the music volume (0.0 to 1.0)

### CE.Audio.SetSFXVolume(volume)
Sets the SFX volume (0.0 to 1.0)

### CE.Audio.AddEffect(handle, name, effect)
Adds an audio effect to a sound instance

### CE.Audio.RemoveEffect(handle, name)
Removes an audio effect from a sound instance by name

### CE.Audio.ClearEffects(handle)
Clears all audio effects from a sound instance
