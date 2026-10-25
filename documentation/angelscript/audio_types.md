# CE::Audio

Registration priority: `100`

## AudioEffect
C++ type: `CE::Core::Audio::AudioEffect`

Flags: `Value`

No description given

### Properties
#### Type
Type: `AudioEffectType`

#### Enabled
Type: `bool`

#### CutoffHz
Type: `float`

#### WetMix
Type: `float`

#### Feedback
Type: `float`

#### DelayMs
Type: `float`

#### DepthMs
Type: `float`

#### RateHz
Type: `float`

#### RoomSize
Type: `float`

#### Damping
Type: `float`

### Behaviours
#### Constructor
No description given

#### Destruct
No description given


## AudioAsset
C++ type: `CE::Audio::Resources::AudioHandle`

Flags: `Value`, `POD`, `AutoGetFlags`

No description given

### Properties
#### handle
Type: `uint32`

### Operators
#### ==
Return type: `bool`

Signature:
```angelscript
const AudioAsset& in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
uint
```

No description given

#### =
Return type: `AudioAsset`

Signature:
```angelscript
AudioAsset & in
```

No description given


## PlayingAudio
C++ type: `CE::Audio::Resources::PlayingAudioHandle`

Flags: `Value`, `POD`, `AutoGetFlags`

No description given

### Properties
#### handle
Type: `uint32`

### Operators
#### ==
Return type: `bool`

Signature:
```angelscript
const PlayingAudio& in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
uint
```

No description given

#### =
Return type: `PlayingAudio`

Signature:
```angelscript
const PlayingAudio & in
```

No description given


## AudioDeviceInfo
C++ type: `CE::Core::Audio::AudioDeviceInfo`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### id
Type: `uint32`

#### stereo
Type: `bool`

#### name
Type: `string`

### Behaviours
#### Construct
No description given

#### Destruct
No description given

### Operators
#### =
Return type: `AudioDeviceInfo`

Signature:
```angelscript
const AudioDeviceInfo & in
```

No description given

#### ==
Return type: `bool`

Signature:
```angelscript
const AudioDeviceInfo & in
```

No description given


## CE::Audio
## Enums
### AudioType
C++ type: `CE::Core::Audio::AudioType`

No description given

Values:

- `SFX`
- `Music`
- `Error`

### AudioEffectType
C++ type: `CE::Core::Audio::AudioEffect::Type`

No description given

Values:

- `LowPass`
- `HighPass`
- `Reverb`
- `Delay`
- `Chorus`
