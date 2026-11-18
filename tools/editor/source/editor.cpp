#include "ce_editor/constants.hpp"
#include "ce_editor/editor.hpp"

#include "ce_runtime/instance/base_instance.hpp"
#include "ce_runtime/common/fs/os_fs/directory_file_provider.hpp"

namespace CE::Editor {
    InstanceRuntimeConf EditorInstance::PrepareRuntimeData()  {
        return InstanceRuntimeConf {
            .vsync = false, // TODO: Make an EditorSettings instead of hardcoding this
            .max_fps = Constants::kMaxFPS,
            .window_title = Constants::kWindowTitle,
            .window_icon_path = Constants::kWindowIconPath,
            .window_width = Constants::kDefaultWindowWidth,
            .window_height = Constants::kDefaultWindowHeight,
            .window_mode = Constants::kDefaultWindowMode,
            .window_resizable = Constants::kWindowResizable,
            .renderer_name = "<unavaible>",
        };
    }

    int EditorInstance::MountData(const std::string& path) {
        mRootMountPointID = mVFS->AddMountPoint<Common::FS::OSFS::DirectoryFileProvider>("/", 100, path);
    }
} 