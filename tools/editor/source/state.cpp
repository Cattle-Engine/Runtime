#include "ce_editor/editor.hpp"
#include "ce_editor/state.hpp"
#include "ce_editor/constants.hpp"

#include "ce_common/tdf.hpp"
#include "ce_common/tracelog.hpp"

#include "ce_runtime/common/fs/file_provider.hpp"
#include "ce_runtime/common/fs/ifile_stream.hpp"
#include "ce_runtime/common/misc/window_mode.hpp"

using CE::Common::FS::VFS::OpenFlags;

namespace CE::Editor {
    State::State(Common::FS::VFS::VFS& vfs, EditorInstance& instance) : mVFS(vfs), mInstance(instance) {}

    Status State::SaveCurrentState() {
        try {
            if (!mVFS.FileExists(kEditorDataPath)) {
                mVFS.CreateFile(kEditorDataPath);
            }

            auto file = mVFS.OpenFile(kEditorDataPath, OpenFlags::Write | OpenFlags::Create);
            if (!file) return Error {
                .message = "Failed to open editor state file",
                .context = "State::SaveCurrentState()",
                .code = std::make_error_code(std::errc::io_error),
            };

            auto stream = Common::FS::VFS::IOStream(std::move(file));
            TDF out;

            out["window"]["win_width"] = mInstance.mWindow->GetWindowSize(Common::WindowMode::Windowed).w;
            out["window"]["win_height"] = mInstance.mWindow->GetWindowSize(Common::WindowMode::Windowed).h;
            out["window"]["win_maximised"] = mInstance.mWindow->IsMaxmimised();

            out.Write(stream);
        } catch (const std::exception& e) {
            return Error {
                .message = e.what(),
                .context = "State::SaveCurrentState()",
            };
        }
        return {};
    }

    Status State::RestoreState() {
        if(!mVFS.FileExists(kEditorDataPath)) {
            CE_LOG(LogLevel::Warn, "[EditorState] Editor state file does not exist. First run?");
            return {};
        }

        auto file = mVFS.OpenFile(kEditorDataPath, OpenFlags::Read);
        if (!file) return Error {
            .message = "Failed to open editor state file",
            .context = "State::RestoreState()",
            .code = std::make_error_code(std::errc::io_error),
        };

        auto stream = Common::FS::VFS::IOStream(std::move(file));
        TDF in = TDF::Read(stream);

        try {
            Common::Window::WindowSize win_size{};
            bool window_maximised = false;;

            const TDF* window = in.Find("window");

            if (window) {
                win_size.w = window->Value<int32_t>(
                    "win_width", Constants::kDefaultWindowWidth
                );

                win_size.h = window->Value<int32_t>(
                    "win_height", Constants::kDefaultWindowHeight
                );

                window_maximised = window->Value<bool>(
                    "win_maximised", false
                );
            }

            mInstance.mWindow->SetWindowSize(win_size, Common::WindowMode::Windowed);
            mInstance.mWindow->MaximiseWindow(window_maximised);
        } catch (const TDF::Error& e) {
            return Error {
                .message = e.what(),
                .context = "State::RestoreState()",
            };  
        }
        return {};
    }
}