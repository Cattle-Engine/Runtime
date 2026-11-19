#pragma once

#include <imgui.h>

namespace CE::Editor::UI {
    class UILayout {
    public:
        void Draw();
        void Reset();

    private:
        void InitialiseDefaultLayout();

        ImGuiID mDockspaceID = 0;
        bool mInitialized = false;
    };
}