#include "engine/assets/skybox_manager.hpp"

#include "engine/common/tracelog.hpp"

namespace CE::Assets::Skyboxes {
    namespace {
        std::shared_ptr<CE::Renderer::Texture> MakeTextureHandle(CE::Renderer::Texture* texture) {
            return std::shared_ptr<CE::Renderer::Texture>(texture, [](CE::Renderer::Texture*) {});
        }
    } // namespace

    SkyBoxManager::SkyBoxManager(CE::Renderer::IRenderer* renderer, CE::VFS::VFS* vfs) {
        gRenderer = renderer;
        gVFS = vfs;
        gErrorTex = gRenderer ? gRenderer->GetErrorTexture() : nullptr;
    }

    std::shared_ptr<CE::Renderer::Texture> SkyBoxManager::LoadFaceTexture(std::string path, bool& isErrorFace) const {
        isErrorFace = true;

        if (!gRenderer || !gVFS) {
            CE_LOG(LogLevel::Error, "[SkyBox Manager] Renderer or VFS is not available");
            return {};
        }

        if (path.empty()) {
            CE_LOG(LogLevel::Error, "[SkyBox Manager] Skybox face path was empty");
            return MakeTextureHandle(gErrorTex);
        }

        if (!gVFS->FileExists(path.c_str())) {
            CE_LOG(LogLevel::Error, "[SkyBox Manager] Missing skybox face: {}", path);
            return MakeTextureHandle(gErrorTex);
        }

        if (auto* texture = gRenderer->LoadTex(path.c_str())) {
            isErrorFace = false;
            return MakeTextureHandle(texture);
        }

        CE_LOG(LogLevel::Error, "[SkyBox Manager] Failed to load skybox face: {}", path);
        return MakeTextureHandle(gErrorTex);
    }

    void SkyBoxManager::Load(std::string frontPath, std::string backPath, std::string leftPath, std::string rightPath,
                             std::string top_path, std::string bottom_path, std::string name) {
        if (name.empty()) {
            CE_LOG(LogLevel::Error, "[SkyBox Manager] Load called with an empty name");
            return;
        }

        const bool wasActive = (gBoundSkyBoxName == name);
        if (gSkyBoxes.contains(name)) {
            Unload(name);
        }

        ManagedSkyBox skybox{};
        bool frontError = false;
        bool backError = false;
        bool leftError = false;
        bool rightError = false;
        bool top_error = false;
        bool bottom_error = false;

        skybox.FrontPath = frontPath;
        skybox.BackPath = backPath;
        skybox.LeftPath = leftPath;
        skybox.RightPath = rightPath;
        skybox.BottomPath = bottom_path;
        skybox.TopPath = top_path;

        skybox.CubeMapData.front = LoadFaceTexture(frontPath, frontError);
        skybox.CubeMapData.back = LoadFaceTexture(backPath, backError);
        skybox.CubeMapData.left = LoadFaceTexture(leftPath, leftError);
        skybox.CubeMapData.right = LoadFaceTexture(rightPath, rightError);
        skybox.CubeMapData.top = LoadFaceTexture(top_path, top_error);
        skybox.CubeMapData.bottom = LoadFaceTexture(bottom_path, bottom_error);

        skybox.IsErrorSkyBox = frontError || backError || leftError || rightError || top_error || bottom_error;
        gSkyBoxes[name] = skybox;

        if (wasActive) {
            Set(name);
        }
    }

    void SkyBoxManager::Set(std::string name) {
        if (!gRenderer) {
            return;
        }

        if (name.empty()) {
            gRenderer->SetSkyBox({});
            gBoundSkyBoxName.clear();
            return;
        }

        auto it = gSkyBoxes.find(name);
        if (it == gSkyBoxes.end()) {
            CE_LOG(LogLevel::Error, "[SkyBox Manager] Tried to set an unloaded skybox: {}", name);
            return;
        }

        gRenderer->SetSkyBox(it->second.CubeMapData);
        gBoundSkyBoxName = name;
    }

    void SkyBoxManager::Unload(std::string name) {
        auto it = gSkyBoxes.find(name);
        if (it == gSkyBoxes.end()) {
            CE_LOG(LogLevel::Error, "[SkyBox Manager] Can not unload a non-existant skybox");
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
        unloadFace(it->second.CubeMapData.top);
        unloadFace(it->second.CubeMapData.bottom);

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
            unloadFace(skybox.CubeMapData.top);
            unloadFace(skybox.CubeMapData.bottom);
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
} // namespace CE::Assets::Skyboxes
