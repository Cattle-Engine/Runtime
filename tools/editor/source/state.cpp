#include "ce_editor/editor.hpp"
#include "ce_editor/state.hpp"

#include "ce_common/tdf.hpp"

#include "ce_runtime/common/fs/file_provider.hpp"
#include "ce_runtime/common/fs/ifile_stream.hpp"
#include "ce_runtime/common/misc/window_mode.hpp"

using CE::Common::FS::VFS::OpenFlags;

namespace CE::Editor {
    State::State(Common::FS::VFS::VFS& vfs, EditorInstance& instance) : mVFS(vfs), mInstance(instance) {}

    Status State::SaveCurrentState() {
        auto file = mVFS.OpenFile("/config/editor_state.tdf", OpenFlags::Write | OpenFlags::Create | OpenFlags::Read);
        if (!file) return Error {
            .message = "Failed to open editor state file",
            .context = "State::SaveCurrentState()",
            .code = std::make_error_code(std::errc::io_error),
        };

        auto stream = Common::FS::VFS::IOStream(std::move(file));
        TDF out;

        out["window"]["win_width_w"] = mInstance.mWindow->GetWindowSize(Common::WindowMode::Windowed).w;
        out["window"]["win_width_w"] = mInstance.mWindow->GetWindowSize(Common::WindowMode::Windowed).h;
        out["window"]["win_maximised"] = mInstance.mWindow->IsMaxmimised();

        out.Write(stream);
    }
}