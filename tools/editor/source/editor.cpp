#include <SDL3/SDL_events.h>
#include <SDL3/SDL_messagebox.h>
#include <algorithm>
#include <exception>
#include <imgui.h>

#include "ce_editor/constants.hpp"
#include "ce_editor/editor.hpp"
#include "ce_editor/state.hpp"
#include "ce_editor/assets/editors/image_viewer.hpp"
#include "ce_editor/ui/proj_fs_browser_win.hpp"

#include "ce_common/tracelog.hpp"

#include "ce_runtime/instance/base_instance.hpp"
#include "ce_runtime/common/fs/os_fs/directory_file_provider.hpp"
#include "ce_runtime/platforms.hpp"
#include "ce_runtime/common/sdl_events.hpp"

namespace CE::Editor {
    InstanceRuntimeConf EditorInstance::PrepareRuntimeData()  {
        return InstanceRuntimeConf {
            .vsync = false, // TODO: Make an EditorSettings instead of hardcoding this
            .max_fps = Constants::kMaxFPS,
            .window_title = Constants::kWindowTitle,
            .window_icon_path = Constants::kWindowIconPath,
            .window_width = Constants::kDefaultWindowWidth,
            .window_height = Constants::kDefaultWindowHeight,
            .window_mode = Constants::kWindowMode,
            .window_resizable = Constants::kWindowResizable,
            .renderer_name = "<unavailable>",
        };
    }

    int EditorInstance::MountData(const std::string& path) {
        mRootMountPointID = mVFS->AddMountPoint<Common::FS::OSFS::DirectoryFileProvider>("/", 100, path);
        mConfigMountPointID = mVFS->AddMountPoint<Common::FS::OSFS::DirectoryFileProvider>("/config", 100, Platforms::GetConfigPath("org.cattle_engine.editor"));
        return 0;
    }

    void EditorInstance::HandleWindowEvents() {
        auto indices = SDL_Events::GetWindowEventIndices(SDL_GetWindowID(mWindow->GetWindow())); // NOLINT

        for (size_t i : indices) {
            const SDL_Event& e = SDL_Events::gEvents[i];

            switch (e.type) { // NOLINT
                case SDL_EVENT_WINDOW_RESIZED: {
                    mWindow->OnWindowResized();
                    break;
                }
            }
        }
    }

    /*
        void EditorInstance::OnInitialise() {
        }
    */

    EditorInstance::EditorInstance(
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

        CE_LOG(CECommon::LogLevel::Debug, "[EditorInstance] Creating asset editor manager and editor factories");
        mAssetEditorManger = std::make_unique<Assets::AssetEditorManager>(*this, *mVFS, "/mime/libmagic_mime.mgc");
        mAssetEditorManger->Register<Assets::ImageViewer>("image/*");

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

    EditorInstance::~EditorInstance() {
        State state(*mVFS, *this);
        CE_LOG(LogLevel::Debug, "[EditorInstance] Saving editor state");
        Status status = state.SaveCurrentState();
        if(!status) {
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_WARNING,
                "CE Editor - Warning", 
                std::string("Failed to save editor state: " + status.GetError().message).c_str(), 
                mWindow->GetWindow()
            );       
        }
        CE_LOG(LogLevel::Debug, "[EditorInstance] Saved editor state");
    }

    bool EditorInstance::OnImGui() {
        mUILayout.Draw();

        for (const auto& win : mUIWindows) {
            win->Draw();
        }

        mMainToolBar.Draw();

        return true;
    }

    Status EditorInstance::LoadProject(fs::path path) {
        auto *it = std::find(mRecentProjects.begin(), mRecentProjects.end(), path);

        if (it != mRecentProjects.end()) {
            std::move_backward(mRecentProjects.begin(), it, it + 1);
        } else {
            std::move_backward(mRecentProjects.begin(), mRecentProjects.end() - 1, mRecentProjects.end());
        }

        mRecentProjects[0] = path;

        // Unmount any previously-loaded project
        if (mProjectMounted) {
            mVFS->Unmount(mProjectMountPointID);
            mProjectMounted = false;
        }

        // Mount the project directory at /project
        mProjectMountPointID = mVFS->AddMountPoint<Common::FS::OSFS::DirectoryFileProvider>(
            "/project", 50, path
        );
        mProjectMounted = true;

        try {
            mProject = std::make_unique<Project>(*mVFS, "/project");
        } catch (const std::exception& e) {
            // Unmount on failure
            mVFS->Unmount(mProjectMountPointID);
            mProjectMounted = false;
            mProjectLoaded = false;
            if (mProject) mProject.reset();
            return Error {
                .message = e.what(),
                .context = "EditorInstance::LoadProject",
            };
        }

        mProjectLoaded = true;
        return {};
    }

    Result<Project&> EditorInstance::GetProject() {
        if (!mProject) return Error {
            .message = "No project loaded",
            .context = "EditorInstance::GetProject()",  
        };

        return *mProject;
    }

    bool EditorInstance::ProjectLoaded() const {
        return mProjectLoaded;
    }
}