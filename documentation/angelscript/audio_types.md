# CE::Audio

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

#### Destruct


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


#### ==
Return type: `bool`

Signature:
```angelscript
uint
```


#### =
Return type: `AudioAsset`

Signature:
```angelscript
AudioAsset & in
```



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


#### ==
Return type: `bool`

Signature:
```angelscript
uint
```


#### =
Return type: `PlayingAudio`

Signature:
```angelscript
const PlayingAudio & in
```



## Enums
### AudioType
C++ type: `CE::Core::Audio::AudioType`

Values:

- `SFX`
- `Music`
- `Error`

### AudioEffectType
C++ type: `CE::Core::Audio::AudioEffect::Type`

Values:

- `LowPass`
- `HighPass`
- `Reverb`
- `Delay`
- `Chorus`
