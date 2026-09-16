# CE::Audio

## Functions
### LoadSound
Return type: `AudioAsset`

Signature:
```angelscript
const string& in filepath, const AudioType& in type
```

Loads an audio file from the VFS

### UnloadSound
Return type: `void`

Signature:
```angelscript
const AudioAsset& in handle
```

Unloads an audio handle

### CreatePlayingAudio
Return type: `PlayingAudio`

Signature:
```angelscript
const AudioAsset& in
```

Creates a PlayingAudio handle

### IsPlaying
Return type: `bool`

Signature:
```angelscript
const PlayingAudio& in
```

Returns false if a PlayAudio's audio is playing audio

### DestroyPlayingAudio
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in
```

Destroys a PlayingAudio handle. If it is playing audio, it is stopped

### PlaySound
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in
```

Plays an audio file from the start

### PauseSound
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in
```

Pauses a playing sound (saves playback position)

### ResumeSound
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in
```

Resumes a playing sound from where playback was last stopped

### SeekSound
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in, float seconds
```

Seeks playback in seconds

### StopSound
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in
```

Stops a playing sound (doesn't save playback position)

### StopAllSounds
Return type: `void`

Stops all playing sounds (doesn't save playback position)

### PauseAllSounds
Return type: `void`

Pauses all playing sounds (saves playback position)

### ResumeAllSounds
Return type: `void`

Resumes all paused sounds from the playback position

### AddEffect
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in, const string& in, const AudioEffect& in
```

Adds an effect to playing audio

### RemoveEffect
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in, const string& in
```

Removes an effect from playing audio

### ClearEffects
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in
```

Clears all effects from playing audio

### SetSoundBus
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in, const string& in bus_name
```

Sets a playing audio's sound bus

### GetSoundBus
Return type: `string`

Signature:
```angelscript
const PlayingAudio& in
```

Gets a PlayingAudio's sound bus name

### SetBusVolume
Return type: `void`

Signature:
```angelscript
const string& in bus_name, float volume
```

Sets a bus volume

### SetBusVoiceLimit
Return type: `void`

Signature:
```angelscript
const string& in bus_name, uint limit
```

Sets how many audio instances can play on that bus

### SetSoundVolume
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in, int volume
```

Sets a playing audio volume

### SetMasterVolume
Return type: `void`

Signature:
```angelscript
float
```

Sets the global volume

### SetMusicVolume
Return type: `void`

Signature:
```angelscript
float
```

Sets the music volume

### SetSFXVolume
Return type: `void`

Signature:
```angelscript
float
```

Sets the music volume

### SetSoundMuted
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in, bool muted
```

Mutes or unmutes a sound

### SetSoundGain
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in, float gain
```

Sets a sounds gain

### SetSoundLabel
Return type: `void`

Signature:
```angelscript
const PlayingAudio& in, const string& in label
```

Sets a sound label

### GetSoundLabel
Return type: `string`

Signature:
```angelscript
const PlayingAudio& in
```

Get a sound label
