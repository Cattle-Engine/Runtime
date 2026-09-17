# CE::Graphics::Render2D::AnimatedTextures

## AnimationInstance
C++ type: `CE::Assets::Animations::AnimationInstanceHandle`

Flags: `Value`, `POD`, `AutoGetFlags`

A handle type for an animation instance.
0 Means invalid.

### Properties
#### id
Type: `uint`


## Functions
### Load
Return type: `bool`

Signature:
```angelscript
const string& in name, const string& in path
```

Loads a animation from the VFS. Returns false if it has failed.
For more infomation on the animation format used check (here)[/documentation/animated_texture_format.md]

### Unload
Return type: `bool`

Signature:
```angelscript
const string& in name
```

Unloads an animated texture. 
If this returns false you probably gave a stale/invalid handle

### CreateInstance
Return type: `AnimationInstance`

Signature:
```angelscript
const string& in animation_name
```

Creates an animation instance. This is basically a playing animation

### DestroyInstance
Return type: `bool`

Signature:
```angelscript
const AnimationInstance& in instance
```

Destroys an animation instance.
If this returns false you've probably given a stale/invalid handle

### PlayAnimation
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in animation, int x, int y, bool loop, bool auto_render
```

Starts an animation.
If ```auto_render``` is true this will make the animation play at the end of 2D rendering.
If it is false you will have to call "DrawFrame" to draw a frame.

### PlayAnimation
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in animation, int x, int y, bool loop, float rotation, bool auto_render
```

Starts an animation with rotation.
Rotation is in radians.
If ```auto_render``` is true this will make the animation play at the end of 2D rendering.
If it is false you will have to call "DrawFrame" to draw a frame.

### SetPlaybackPosition
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in, uint frame
```

Seeks to the specified frame on an animation instance

### SetPosition
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in, int x, int y, float rotation = 0.0f
```

Sets the position of an animation instance. Rotation is in radians

### SeekFrame
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in, uint frame
```

Seeks the current frame of an animation instance

### SetDrawMode
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in, bool auto_render
```

Sets the draw mode (draw at the end of 2D rendering or you manually draw).

### SetLooping
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in, bool loop
```

Sets if an animation should loop

### SetTint
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in, Colour& in colour
```

Sets the tint of the animation

### PauseAnimation
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in
```

Pauses an animation. This saves the playback position.

### StopAnimation
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in
```

Stops an animation. This resets the playback position to 0

### DrawFrame
Return type: `void`

Signature:
```angelscript
const AnimationInstance& in
```

This is when for when auto_render is set to false and you want to draw an animation frame.
