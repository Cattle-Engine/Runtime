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
    uint64_t PatchOffset = 0;
};

struct CEUpdatePatchedFileChunk {
    uint64_t ChunkID;
    void* Data;
    uint8_t CompressionType;
    uint64_t CompressedSize;
    uint64_t UnCompressedSize;
    uint32_t Crc32;
};

struct CEUpdatePatchedFile {
    std::vector<CEUpdatePatchedFileChunk> Chunks; // Any new chunks added must be monotonic to the existing file chunks
    uint32_t FileID;
};

struct CEUpdateReplacedFile {
    uint32_t FileID;
    int64_t DataModified;
    std::vector<CEUpdatePatchedFileChunk> Chunks;
};
```

# Refactor audio bus system
Maybe use handles instead of strings for audio buses

# Make the documentation in the idl yaml better

```yaml
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

# Better idea for the ECS idea in TODO.md
So, ECS's are fun and all but I have a much better idea,
that doesn't mean the entire engine turns into a fkin ECS framework.
We take the bulk data storage idea of an ECS BUT, 

For the main class; ACCS: Actor Component Control System

For components do this

```cpp
struct ComponentTypeID {
    uint32_t id
}

class IActorComponent {
    public:
        IActorComponent() = default;
        ~IActorComponent() = default;

        virtual ComponentTypeID GetTypeID() const = 0;
};
```

and then for their controller

Then we have an actor class:

```