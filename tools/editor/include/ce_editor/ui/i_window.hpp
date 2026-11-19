#pragma once

#include <string>

namespace CE::Editor {
    class EditorInstance;

    namespace UI {
        // provides a standard API to draw windows
        class IWindow {
            public:
                IWindow(EditorInstance& instance) : mEditorInstance(instance) {}
                virtual ~IWindow() = default;

                virtual std::string GetWindowName() = 0;
                virtual void Draw() = 0;
                void SetWindowOpen(bool open) { mWindowOpen = open; };
                bool IsWindowOpen() const { return mWindowOpen; };
            protected:
                bool mWindowOpen = true; // NOLINT
                EditorInstance& mEditorInstance; // NOLINT
        };
    }
}