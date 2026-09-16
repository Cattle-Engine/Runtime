# CE::Settings

## SettingsInfo
C++ type: `CE::Settings::SettingsInfo`

Flags: `Value`, `AutoGetFlags`

No description given

### Properties
#### window_height
Type: `int`

#### window_width
Type: `int`

#### max_fps
Type: `int`

#### enable_vsync
Type: `bool`

#### renderer_name
Type: `string`

#### window_mode
Type: `CE::Window::WindowMode`

#### master_volume
Type: `float`

#### music_volume
Type: `float`

#### sfx_volume
Type: `float`

### Behaviours
#### Construct

#### Destructor


## Functions
### GetSettingsInfo
Return type: `SettingsInfo`

Gets the current SettingsInfo

### SetSettingsInfo
Return type: `void`

Signature:
```angelscript
const SettingsInfo& in
```

Sets a SettingsInfo to the settings manager

### ReloadSettings
Return type: `bool`

Reloads settings from disk and applies them

### FlushSettings
Return type: `void`

Flushes the current set SettingsInfo to disk

### GetInteger
Return type: `int`

Signature:
```angelscript
const string& in key, const string& in section, const int fallback
```

Gets a 32bit integer from settings

### GetFloat
Return type: `float`

Signature:
```angelscript
const string& in key, const string& in section, const float fallback
```

Gets a 32bit float from settings

### GetBool
Return type: `bool`

Signature:
```angelscript
const string& in key, const string& in section, const bool fallback
```

Gets a bool from settings

### GetString
Return type: `string`

Signature:
```angelscript
const string& in key, const string& in section, const string& in fallback
```

Gets a string from settings

### SetInteger
Return type: `void`

Signature:
```angelscript
const string& in key, const string& in section, const int value
```

Sets an int into settings

### SetFloat
Return type: `void`

Signature:
```angelscript
const string& in key, const string& in section, const float value
```

Sets a float into settings

### SetBool
Return type: `void`

Signature:
```angelscript
const string& in key, const string& in section, const bool value
```

Sets a boolean into settings

### SetString
Return type: `void`

Signature:
```angelscript
const string& in key, const string& in section, const string& in value
```

Sets a string into settings
