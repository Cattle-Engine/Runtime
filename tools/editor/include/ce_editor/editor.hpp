#pragma once

#include <array>
#include <string>
#include <filesystem>
#include <memory>
#include <vector>

#include <SDL3/SDL.h>

#include "ce_runtime/common/fs/vfs.hpp"
#include "ce_runtime/common/misc/gameinfo.hpp"
#include "ce_runtime/instance/base_instance.hpp"
#include "ce_runtime/settings.hpp"

#include "ce_editor/state.hpp"
#include "ce_editor/ui/layout.hpp"
#include "ce_editor/ui/proj_fs_browser_win.hpp"
#include "ce_editor/ui/toolbar.hpp"
#include "ce_editor/ui/i_window.hpp"

namespace CE::Editor {
    namespace fs = std::filesystem;

    class EditorInstance : public ::CE::BaseInstance {
        public:
            static constexpr int kMaxRecentProjects = 5;    

            EditorInstance(
                bool debugmode,
                Renderer::GPUDeviceHandle& gpudevice,
                EngineArguements args
            ) : mMainToolBar(*this), mUILayout() {
                mUIWindows.push_back(
                    std::make_unique<UI::ProjectFSBrowser>(*this)
                );

                this->InitialiseBase(
                    debugmode,
                    gpudevice,
                    args,
                    std::string(std::string(SDL_GetBasePath()) + "/editor_data")
                );
            }

            ~EditorInstance();

            std::array<std::string, kMaxRecentProjects>& GetRecentProjects() {
                return mRecentProjects;
            }

            Status LoadProject(fs::path path);
            Result<fs::path> GetProjectPath() const;
            bool ProjectLoaded() const;

            std::vector<std::unique_ptr<UI::IWindow>>& GetUIWindows() { return mUIWindows; }
        protected:
            GameInfo& GetGameInfo() override {
                static GameInfo info{};
                return info;
            }

            Settings::SettingsManager& GetSettingsManager() override {
                return mSettings;
            }

            bool OnImGui() override;
            
            void OnInitialise() override;

            InstanceRuntimeConf PrepareRuntimeData()  override;
            int MountData(const std::string& path) override;
            void HandleWindowEvents() override;
        private:

            friend class State;

            Common::FS::VFS::MountPoint::MountID mRootMountPointID{0};
            Common::FS::VFS::MountPoint::MountID mConfigMountPointID{0};

            Settings::SettingsManager mSettings = Settings::SettingsManager("org.cattle_engine.editor");
            UI::MainViewPortToolBar mMainToolBar;
            UI::UILayout mUILayout;

            std::vector<std::unique_ptr<UI::IWindow>> mUIWindows;

            std::array<std::string, kMaxRecentProjects> mRecentProjects;

            fs::path mCurrentProjectPath;
            bool mProjectLoaded = false;
    };
}