#pragma once

#include <array>
#include <string>
#include <filesystem>
#include <memory>
#include <vector>

#include <SDL3/SDL.h>

#include "ce_runtime/common/fs/vfs.hpp"
#include "ce_runtime/common/misc/gameinfo.hpp"
#include "ce_runtime/common/window.hpp"
#include "ce_runtime/instance/base_instance.hpp"
#include "ce_runtime/settings.hpp"

#include "ce_editor/state.hpp"
#include "ce_editor/ui/layout.hpp"
#include "ce_editor/ui/proj_fs_browser_win.hpp"
#include "ce_editor/ui/toolbar.hpp"
#include "ce_editor/ui/i_window.hpp"
#include "ce_editor/project.hpp"
#include "ce_editor/ui/theme_manager.hpp"

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
                mThemeManager = std::make_unique<UI::ThemeManager>(*mRenderer, *mVFS, "org.cattle_engine.editor_themes.system_default");

                State state(*mVFS, *this);

                Status status = state.RestoreState();
                if (!status) {
                    SDL_ShowSimpleMessageBox(
                        SDL_MESSAGEBOX_WARNING,
                        "CE Editor - Warning", 
                        std::string("Failed to restore editor state: " + status.GetError().message).c_str(), 
                        mWindow->GetWindow()
                    );
                }
            }

            ~EditorInstance();

            std::array<std::string, kMaxRecentProjects>& GetRecentProjects() { return mRecentProjects; }
            [[nodiscard]] Status LoadProject(fs::path path);
            [[nodiscard]] Result<Project&> GetProject();
            bool ProjectLoaded() const;

            UI::ThemeManager& GetThemeManager() { return *mThemeManager; }
            Common::Window& GetWindow() { return *mWindow; }
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
            std::unique_ptr<UI::ThemeManager> mThemeManager;
            std::unique_ptr<Project> mProject;
            std::array<std::string, kMaxRecentProjects> mRecentProjects;
            bool mProjectLoaded = false;
    };
}