# CE Engine - AI Coding Agent Instructions

## Project Overview
**Cattle Engine (ce_engine)**: A C++20 game engine built on raylib with cross-platform support (Linux, macOS, Windows) and a plugin architecture for extensibility. Focus areas: asset management, UI widgets, Lua scripting integration, and plugin system.

## Architecture

### Core Components
- **Bootstrap** ([source/bootstrap.cpp](source/bootstrap.cpp)): Initializes paths, window, settings, and plugins. Handles platform-specific directory setup (XDG_CACHE_HOME on Linux, Library/Caches on macOS, AppData on Windows).
- **Engine Main Loop** ([source/engine/engine_main.cpp](source/engine/engine_main.cpp)): Simple `CE::Engine::Main()` that runs input polling, drawing, and widget updates within a raylib loop.
- **Module System** ([source/engine/modules.cpp](source/engine/modules.cpp)): Dynamic library loading for plugins (cross-platform via LoadLibrary/dlopen pattern). Framework scaffolded but not yet implemented.
- **Globals** ([include/globals.hpp](include/globals.hpp)): Centralized `CE::Global` struct + inline globals for game state, mouse position, debug flags, and paths.

### Asset Management
- **Textures** ([source/engine/Asset_Manager/textures.cpp](source/engine/Asset_Manager/textures.cpp)): `std::unordered_map`-based texture cache. Auto-loads on first Draw() call if missing; returns error texture (magenta checkerboard) for missing/failed assets. Key pattern: lazy loading.
- **TDF Format** ([include/common/tdf.hpp](include/common/tdf.hpp)): Custom binary file format (typed key-value store) supporting primitives, arrays, and serialization. Used for saves/game data.

### UI System
- **Widgets** ([include/engine/UI.hpp](include/engine/UI.hpp)): Button class with state management (hovering, clicked). Base pattern for future widget expansion.

### Configuration
- **Settings** ([source/engine/settings.cpp](source/engine/settings.cpp)): Persists fullscreen, volume levels via minini (INI parser). Stores in platform-standard config dirs. Loads/creates on startup.

## Key Patterns & Conventions

### C++ Namespace Organization
All engine code in `CE::` namespace with subnamespaces: `CE::Engine::`, `CE::Assets::Textures::`, `CE::UI::Widgets::`. Use `extern "C"` for C libraries (raylib, minini, TDF).

### Error Handling
- Raylib's `TraceLog()` with conditional levels (`LOG_ERROR`, `LOG_WARNING`, `LOG_INFO`)
- Debug mode via `--debug` CLI flag sets `CE::debug = true`
- Error textures and graceful fallbacks in asset system (e.g., missing texture returns ErrorTexture)
- Use `ShowError()` for critical bootstrap failures

### Resource Management
- Textures stored in `CE::Global.data_path` (resolved at runtime by OS-specific logic)
- Settings in `CE::Global.settings_path`
- Manual unloading: `Unload()` removes from cache and calls raylib cleanup
- No RAII wrappers yet; potential improvement area

### Cross-Platform Considerations
- Preprocessor flags: `_WIN32`, `__linux__`, `__APPLE__`, `_WIN64`
- Custom macros for DLL export in plugin API: `CE_EXPORT` handles Windows `__declspec(dllexport)` vs GCC visibility
- Makefile detects OS and uses appropriate compiler (mingw32-g++ on Windows, g++ on Linux/macOS)

## Plugin Architecture

### Plugin Lifecycle
1. Engine loads .dll/.so via `LOAD_LIB()` macro (wraps LoadLibrary/dlopen)
2. Engine calls `CE_PluginInfo* CE_PluginInfo()` to query capabilities
3. Engine calls `void CE_PluginInit(CE_Funcs*, Globals*)` to hand pointers
4. Plugin hooks into UI callbacks: `CE_PluginUpdateUI_InGame()`, `CE_PluginUpdateUI_MainMenu()`, etc.
5. Plugin can inject Lua functions if `CE_Lua_Injects` capability set

### Plugin API Contract
- Capabilities use bitwise flags ([Plugin-test/CE_API.hpp](Plugin-test/CE_API.hpp)): `CE_InGame`, `CE_UI_Main_Menu`, `CE_UI_InGame`, `CE_UI_Pause_Menu`, `CE_Lua_Injects`
- Plugins receive global pointers (currently empty structs `Globals` and `CE_Funcs`; design in progress)

## Build System

### Compilation
```bash
make              # Build ce_engine (detects OS, sets PLATFORM_FLAGS)
make clean        # Remove build artifacts
```
- Source autodiscovery: `$(shell find source -name '*.cpp' -o -name '*.c')`
- Include paths: `-Iinclude`, third-party headers included directly
- Standard: C++20, warnings on (`-Wall -Wextra`)

### Runtime Flags
- `--debug`: Enables debug console, trace logging, triggers `CE::debug = true`
- `--plugin-logs`: Shows plugin-specific logs (controlled by `CE::showPluginLogs`)

## Developer Workflows

### Adding a New Asset Type
1. Create namespace in `include/engine/assets/` (e.g., `Audio.hpp`)
2. Implement with unordered_map cache pattern like `textures.cpp`
3. Expose Init/Load/Get/Unload functions
4. Call Init early in bootstrap if stateful initialization needed

### Adding a UI Widget
1. Define class in `include/engine/UI.hpp` with state members (position, size, colors)
2. Implement `update()` method that checks mouse input (use `CE::MousePos`)
3. Call `update()` in main loop before drawing
4. Use raylib draw functions for rendering

### Extending Settings
1. Add field to `CE::Setting` struct in [include/globals.hpp](include/globals.hpp)
2. Add ini_getl/ini_putl/ini_getbool calls in [source/engine/settings.cpp](source/engine/settings.cpp)
3. Ini keys use two-level format: `[Section]` then `key = value`

## Critical Files to Know

| File | Purpose |
|------|---------|
| [include/globals.hpp](include/globals.hpp) | Global state, engine constants, Settings struct |
| [source/bootstrap.cpp](source/bootstrap.cpp) | Path setup, window init, plugin loading init |
| [source/engine/Asset_Manager/textures.cpp](source/engine/Asset_Manager/textures.cpp) | Texture loading/caching pattern |
| [include/common/tdf.hpp](include/common/tdf.hpp) | Save file format definition |
| [Plugin-test/CE_API.hpp](Plugin-test/CE_API.hpp) | Plugin ABI contract |
| [source/engine/settings.cpp](source/engine/settings.cpp) | INI config persistence |

## Common Gotchas

1. **Raylib C API**: Always wrap in `extern "C"` when including; raylib.h headers conflict with windows.h macros—custom logging in modules.cpp avoids this.
2. **Lazy Texture Loading**: Draw() auto-loads missing textures; explicit Load() pre-caches. Unload() is manual.
3. **Path Construction**: Always use OS-specific setup from bootstrap; never hardcode paths relative to binary.
4. **Plugin API Stubs**: `CE_Funcs` and `Globals` structs are placeholders; expand with function pointers/state as needed.
