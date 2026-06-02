#include "engine/assets/skybox_manager.hpp"

#include "engine/common/tracelog.hpp"

namespace CE::Assets::Skyboxes {
    namespace {
        std::shared_ptr<CE::Renderer::Texture> MakeTextureHandle(CE::Renderer::Texture* texture) {
            return std::shared_ptr<CE::Renderer::Texture>(texture, [](CE::Renderer::Texture*) {});
        }
    }

    SkyBoxManager::SkyBoxManager(CE::Renderer::IRenderer* renderer, CE::VFS::VFS* vfs) {
        gRenderer = renderer;
        gVFS = vfs;
        gErrorTex = gRenderer ? gRenderer->GetErrorTexture() : nullptr;
    }

    std::shared_ptr<CE::Renderer::Texture> SkyBoxManager::LoadFaceTexture(const char* path, bool& isErrorFace) const {
        isErrorFace = true;

        if (!gRenderer || !gVFS) {
            CE::Log(LogLevel::Error, "[SkyBox Manager] Renderer or VFS is not available");
            return {};
        }

        if (!path || path[0] == '\0') {
            CE::Log(LogLevel::Error, "[SkyBox Manager] Skybox face path was empty");
            return MakeTextureHandle(gErrorTex);
        }

        if (!gVFS->FileExists(path)) {
            CE::Log(LogLevel::Error, "[SkyBox Manager] Missing skybox face: {}", path);
            return MakeTextureHandle(gErrorTex);
        }

        if (auto* texture = gRenderer->LoadTex(path)) {
            isErrorFace = false;
            return MakeTextureHandle(texture);
        }

        CE::Log(LogLevel::Error, "[SkyBox Manager] Failed to load skybox face: {}", path);
        return MakeTextureHandle(gErrorTex);
    }

    void SkyBoxManager::Load(const char* frontPath, const char* backPath, const char* leftPath, const char* rightPath, const char* name) {
        if (!name || name[0] == '\0') {
            CE::Log(LogLevel::Error, "[SkyBox Manager] Load called with an empty name");
            return;
        }

        const bool wasActive = (gBoundSkyBoxName == name);
        if (gSkyBoxes.contains(name)) {
            Unload(name);
        }

        ManagedSkyBox skybox {};
        bool frontError = false;
        bool backError = false;
        bool leftError = false;
        bool rightError = false;

        skybox.FrontPath = frontPath ? frontPath : "";
        skybox.BackPath = backPath ? backPath : "";
        skybox.LeftPath = leftPath ? leftPath : "";
        skybox.RightPath = rightPath ? rightPath : "";

        skybox.CubeMapData.front = LoadFaceTexture(frontPath, frontError);
        skybox.CubeMapData.back = LoadFaceTexture(backPath, backError);
        skybox.CubeMapData.left = LoadFaceTexture(leftPath, leftError);
        skybox.CubeMapData.right = LoadFaceTexture(rightPath, rightError);

        skybox.IsErrorSkyBox = frontError || backError || leftError || rightError;
        gSkyBoxes[name] = skybox;

        if (wasActive) {
            Set(name);
        }
    }

    void SkyBoxManager::Set(const char* name) {
        if (!gRenderer) {
            return;
        }

        if (!name || name[0] == '\0') {
            gRenderer->SetSkyBox({});
            gBoundSkyBoxName.clear();
            return;
        }

        auto it = gSkyBoxes.find(name);
        if (it == gSkyBoxes.end()) {
            CE::Log(LogLevel::Error, "[SkyBox Manager] Tried to set an unloaded skybox: {}", name);
            return;
        }

        gRenderer->SetSkyBox(it->second.CubeMapData);
        gBoundSkyBoxName = name;
    }

    void SkyBoxManager::Unload(const char* name) {
        auto it = gSkyBoxes.find(name ? name : "");
        if (it == gSkyBoxes.end()) {
            CE::Log(LogLevel::Error, "[SkyBox Manager] Can not unload a non-existant skybox");
            return;
        }

        if (!gBoundSkyBoxName.empty() && gBoundSkyBoxName == it->first) {
            if (gRenderer) {
                gRenderer->SetSkyBox({});
            }
            gBoundSkyBoxName.clear();
        }

        auto unloadFace = [this](const std::shared_ptr<CE::Renderer::Texture>& face) {
            if (!face || face.get() == gErrorTex) {
                return;
            }
            if (gRenderer) {
                gRenderer->UnloadTex(face.get());
            }
        };

        unloadFace(it->second.CubeMapData.front);
        unloadFace(it->second.CubeMapData.back);
        unloadFace(it->second.CubeMapData.left);
        unloadFace(it->second.CubeMapData.right);

        gSkyBoxes.erase(it);
    }

    void SkyBoxManager::UnloadAll() {
        if (gRenderer && !gBoundSkyBoxName.empty()) {
            gRenderer->SetSkyBox({});
        }

        for (auto& [name, skybox] : gSkyBoxes) {
            (void)name;
            auto unloadFace = [this](const std::shared_ptr<CE::Renderer::Texture>& face) {
                if (!face || face.get() == gErrorTex) {
                    return;
                }
                if (gRenderer) {
                    gRenderer->UnloadTex(face.get());
                }
            };

            unloadFace(skybox.CubeMapData.front);
            unloadFace(skybox.CubeMapData.back);
            unloadFace(skybox.CubeMapData.left);
            unloadFace(skybox.CubeMapData.right);
        }

        gSkyBoxes.clear();
        gBoundSkyBoxName.clear();
    }

    int SkyBoxManager::Debug_LoadedSkyBoxesCount() const {
        return static_cast<int>(gSkyBoxes.size());
    }

    int SkyBoxManager::Debug_LoadedSkyBoxesNoError() const {
        int count = 0;
        for (const auto& [name, skybox] : gSkyBoxes) {
            (void)name;
            if (!skybox.IsErrorSkyBox) {
                ++count;
            }
        }
        return count;
    }

    int SkyBoxManager::Debug_LoadedSkyBoxesError() const {
        int count = 0;
        for (const auto& [name, skybox] : gSkyBoxes) {
            (void)name;
            if (skybox.IsErrorSkyBox) {
                ++count;
            }
        }
        return count;
    }

    std::string SkyBoxManager::Debug_GetBoundSkyBoxName() const {
        return gBoundSkyBoxName.empty() ? "None" : gBoundSkyBoxName;
    }

    std::vector<SkyBoxManager::DebugSkyBoxInfo> SkyBoxManager::Debug_GetSkyBoxes() const {
        std::vector<DebugSkyBoxInfo> out;
        out.reserve(gSkyBoxes.size());

        for (const auto& [name, skybox] : gSkyBoxes) {
            DebugSkyBoxInfo info{};
            info.name = name;
            info.frontPath = skybox.FrontPath;
            info.backPath = skybox.BackPath;
            info.leftPath = skybox.LeftPath;
            info.rightPath = skybox.RightPath;
            info.isErrorSkyBox = skybox.IsErrorSkyBox;
            info.isActive = (gBoundSkyBoxName == name);
            info.cubeMap = skybox.CubeMapData;
            out.push_back(info);
        }

        return out;
    }

    SkyBoxManager::~SkyBoxManager() {
        UnloadAll();
        gErrorTex = nullptr;
    }
}
