okay for the texture manager gonna do this for the texture handle to avoid having to do a unordered map lookup everyframe

```cpp
struct TextureHandleState {
    Texture* Texture = nullptr;
};```

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