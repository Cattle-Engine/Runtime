#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>

#include "ce_runtime/common/fs/vfs.hpp"
#include "ce_runtime/common/misc/gameinfo.hpp"
#include "ce_runtime/instance/base_instance.hpp"
#include "ce_runtime/settings.hpp"

#include "ce_editor/state.hpp"

namespace CE::Editor {
    class EditorInstance : public ::CE::BaseInstance {
        public:
            EditorInstance(
                bool debugmode,
                Renderer::GPUDeviceHandle& gpudevice,
                EngineArguements args
            ) {
                this->InitialiseBase(
                    debugmode,
                    gpudevice,
                    args,
                    std::string(std::string(SDL_GetBasePath()) + "/editor_data")
                );
            }

            ~EditorInstance();
        protected:
            GameInfo& GetGameInfo() override {
                static GameInfo info{};
                return info;
            }

            Settings::SettingsManager& GetSettingsManager() override {
                return mSettings;
            }

            bool OnImGui() override {
                ImGui::Begin("Hi from the editor!");

                ImGui::End();
                return true;
            }
            
            void OnInitialise() override;

            InstanceRuntimeConf PrepareRuntimeData()  override;
            int MountData(const std::string& path) override;
            void HandleWindowEvents() override;
        private:
            friend class State;
            Common::FS::VFS::MountPoint::MountID mRootMountPointID{0};
            Common::FS::VFS::MountPoint::MountID mConfigMountPointID{0};
            Settings::SettingsManager mSettings = Settings::SettingsManager("org.cattle_engine.editor");
    };
}