#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/rendering/renderer.hpp"

namespace CE::Assets::Skyboxes {
    class SkyBoxManager {
      public:
        struct DebugSkyBoxInfo {
            std::string name;
            std::string frontPath;
            std::string backPath;
            std::string leftPath;
            std::string rightPath;
            bool isErrorSkyBox = false;
            bool isActive = false;
            CE::Renderer::CubeMap cubeMap{};
        };

        SkyBoxManager(CE::Renderer::IRenderer* renderer, CE::VFS::VFS* vfs);

        void Load(std::string frontPath, std::string backPath, std::string leftPath, std::string rightPath,
                  std::string top_path, std::string bottom_path, std::string name);
        void Set(std::string name);
        void Unload(std::string name);
        void UnloadAll();

        int Debug_LoadedSkyBoxesCount() const;
        int Debug_LoadedSkyBoxesNoError() const;
        int Debug_LoadedSkyBoxesError() const;
        std::string Debug_GetBoundSkyBoxName() const;
        std::vector<DebugSkyBoxInfo> Debug_GetSkyBoxes() const;

        ~SkyBoxManager();

      private:
        struct ManagedSkyBox {
            bool IsErrorSkyBox = false;
            std::string FrontPath;
            std::string BackPath;
            std::string LeftPath;
            std::string RightPath;
            std::string TopPath;
            std::string BottomPath;
            CE::Renderer::CubeMap CubeMapData{};
        };

        std::shared_ptr<CE::Renderer::Texture> LoadFaceTexture(std::string path, bool& isErrorFace) const;

        CE::Renderer::IRenderer* gRenderer = nullptr;
        CE::VFS::VFS* gVFS = nullptr;
        CE::Renderer::Texture* gErrorTex = nullptr;
        std::unordered_map<std::string, ManagedSkyBox> gSkyBoxes;
        std::string gBoundSkyBoxName;
    };
} // namespace CE::Assets::Skyboxes
