# Cattle Engine - NO BULLSHIT

It's in a somewhat usable state? 
Cattle engine (abbreviated to CE), is a general 2D game engine currently using SDL3, it uses AngelScript for the actual game making stuff with a few custom extension (a custom import/export system) the docs for all this can be found inside [docs.md](docs.md).

Documentation will be moved to the other repo "Documentation" but for now it'll be in this repo.

It has experimental 3D support not the best but it works!

Licences for third party stuff can be found in [/thirdparty/README.md](thirdparty/README.md)

## Building
Primary workflow:

- Configure: `cmake -S . -B build`
- Build: `cmake --build build`

Manual helper entrypoint:

- Full build: `python3 tools/build/build.py`
- Bootstrap only: `python3 tools/build/build.py bootstrap`
- Clean generated files for one build dir: `python3 tools/build/build.py clean-generated --build-dir build`
- Remove a build dir completely: `python3 tools/build/build.py clean --build-dir build`
- Windows bootstrap without system Python: `powershell -ExecutionPolicy Bypass -File tools/bootstrap/bootstrap.ps1`

CMake clean helper:

- Generated files only: `cmake --build build --target ce_clean_generated`

## ROADMAP!

> [!NOTE]
> For more information about what's happening, check [TODO.md](TODO.md).

- [x] Core stuff
- [x] Angelscript API
- [x] Audio support
- [ ] Plugins
- [ ] Data & texture/audio compression
- [ ] Stabilise everything
