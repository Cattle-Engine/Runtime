okay for the texture manager gonna do this for the texture handle to avoid having to do a unordered map lookup everyframe

```cpp
struct TextureHandleState {
    Texture* Texture = nullptr;
};
```

TextureHandle owns a shared_ptr to that state and TextureEntry holds the same state. 
```cpp
struct TextureEntry {
    std::shared_ptr<TextureHandleState> State;
    Texture* Resource = nullptr;
    uint32_t RefCount = 0;
    std::string Path;
    bool IsError = false;
    bool IsPendingUnload = false;
};
```
on unloading we do this ```entry->State->Texture = nullptr;``` and when the refcount goes to zero we actually unload the texture

# Update system

Okay so due to having the VFS it makes it easy enough to have an update system. I'm thinking of using a seperate file format for updates.
Also make it so in Gameinfo.txt we have required integers for the game version.
For updates themselves I'm thinking of a deltapatch system where we don't ship an entire new updated file only the parts that changed.

So the path(s) for updates are defined in Gameinfo.txt under:
```ini
[Gameinfo]
update_paths = {"./updates"}
```
(Dot paths are relative to the base game data)


For the header of the file format I shall do: 

```cpp
struct CEUpdateHeader {
    char Magic[6] = {'C', 'E', 'U', 'P', 'P', '\0'}; // at 0x00
    uint32_t TargetGameMajorVersion = 1; // version of current update file
    uint32_t TargetGameMinorversion = 2;
    uint32_t TargetGamePatchVersion = 0;

    uint32_t RequiredGameMajorVersion = 1; // require the 1.1.0 update and if not found error 
    uint32_t RequiredGameMinorversion = 1;
    uint32_t RequiredGamePatchVersion = 0;

    uint32_t UpdateFormatVersion = 1; // incase I ever update the format
    uint8_t reserved[50];
};
```
After the header we have just a block of THESE

```cpp
enum class CEUpdateType : uint8_t {
    AddFile = 0,
    RemoveFile = 1,
    ReplaceFile = 2,
    PatchFile = 3
};

struct CEUpdate {
    uint8_t UpdateType = 0; // gets casted to an CEUpdateType
    uint32_t PatchLength = 0;
    
};
```

# How games gonna be ran with CE

For windows, linux and mac (if I ever support) the engine and game shall be split into dis
```
ce_runtime.dll
game.exe
```
game.exe loads ce_runtime.dll manually and calls C api functions to load the engine and create an instance.
At the logical end of the file some core info is (base game data, ce_runtime.dll name as it can be anything) as drum roll please, TDF XD

# Namespace refactor program
Just a small C++ program that uses clangs libtooling to help with refactoring stuff from 1 namespace to another

# Refactor audio bus system
Maybe use handles instead of strings for audio buses

# Make the documentation in the idl yaml better

```
- Name: LoadSound
  ReturnType: void
  Signature: const string& in filepath, const AudioType& in type, AudioAsset& out handle

  Description: Loads an audio file from the VFS

  Parameters:
    filepath: Path to the audio file in the VFS
    type: Type of audio asset to load
    handle: Receives the loaded audio asset

  Returns: Nothing

  Since: 1.4
  Category: Audio
  ThreadSafety: MainThread
  Notes:
    - The returned handle remains valid until released.
    - The file must exist in the VFS.

  Function: |
    if (!mRuntime.mAudioManager) return;
    arg2 = mRuntime.mAudioManager->LoadSound(arg0, arg1);
```