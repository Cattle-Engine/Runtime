# Files required by the CE loader
The CE loader only requires 2 files:

```
path.cfg
data.tcf
```

path.cfg MUST point to the base game-data file (in-this case data.tcf), relative to the executable location.
```
data.tcf
```
anything else inside path.cfg is ignored.

# Files required by the CE runtime  
Now the CE runtime only requires a base game-data file on the actual filesytem (as of now),
it does however require a few more files inside the base game-data file, those being:

```
Gameinfo.txt
code.bin
```

## Gameinfo.txt Configuration Reference
### Required settings

These 11 keys are REQUIRED

| Section | Key | Data Type | Example | Purpose |
|---|---|---|---|---|
| `[Gameinfo]` | `Game_Name` | String | `My Game` | Name of the game. |
| `[Gameinfo]` | `Game_Version` | String | `1.0.0` | Base game version. |
| `[Gameinfo]` | `Scripting_Startup_File` | String | `startup.as` | Startup script filename, made by ce_scriptc |
| `[Graphics]` | `Window_Width` | Integer | `1280` | Initial window width in pixels. |
| `[Graphics]` | `Window_Height` | Integer | `720` | Initial window height in pixels. |
| `[Graphics]` | `Window_Title` | String | `My Game` | Window title displayed by the operating system. |
| `[Graphics]` | `Max_FPS` | Integer | `144` | Maximum frame rate setting. |
| `[Graphics]` | `Renderer` | String | `Vulkan` | Renderer selection. Vulkan and Software are only supported rn|
| `[Graphics]` | `Enable_VSync` | Boolean | `true` | Enables or disables vertical synchronization. |
| `[Graphics]` | `Window_Mode` | Integer | `2` | Window mode: `0` = Fullscreen, `1` = Borderless, `2` = Windowed. |
| `[Graphics]` | `Resizable_Window` | Boolean | `true` | Allows the user to resize the window. |

### Optional but useful settings

These 7 keys aren't required but are useful

| Section | Key | Data Type | Default / Example | Purpose |
|---|---|---|---|---|
| `[Gameinfo]` | `Window_Icon` | String | `assets/icon.png` | Path to the window icon. |
| `[Graphics]` | `No_Render_On_Focus_Lost` | Boolean | `true` | Pauses rendering when the window loses focus. |
| `[Graphics]` | `No_Update_On_Focus_Lost` | Boolean | `true` | Pauses updates when the window loses focus. |
| `[Graphics]` | `Max_Window_Width` | Integer | `3840` | Maximum window width in pixels. |
| `[Graphics]` | `Max_Window_Height` | Integer | `2160` | Maximum window height in pixels. |
| `[Graphics]` | `Min_Window_Width` | Integer | `640` | Minimum window width in pixels. |
| `[Graphics]` | `Min_Window_Height` | Integer | `360` | Minimum window height in pixels. |


### An example Gameinfo.txt

```ini
[Gameinfo]
Game_Name = My Game
Game_Version = 1.0.0
Scripting_Startup_File = startup.as
Window_Icon = images/icon.png

[Graphics]
Window_Width = 1280
Window_Height = 720
Window_Title = My Game
Max_FPS = 144
Renderer = Vulkan
Enable_VSync = true
Window_Mode = 2
Resizable_Window = true

No_Render_On_Focus_Lost = true
No_Update_On_Focus_Lost = true

Min_Window_Width = 640
Min_Window_Height = 360
Max_Window_Width = 3840
Max_Window_Height = 2160
```