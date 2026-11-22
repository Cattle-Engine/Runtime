#pragma once

#include <imgui.h>

#include <array>
#include <string>
#include <filesystem>
#include <vector>

#include "ce_common/result.hpp"
#include "ce_runtime/rendering/renderer.hpp"
#include "ce_runtime/common/fs/vfs.hpp"

namespace CE::Editor::UI {
    enum class Icon {
        Undo,
        Redo,
        Count,
    };

    struct Theme {
        // reverse domain style, eg; org.cattle_engine.editor_themes.dark
        // also used in ThemeManager::LoadTheme
        std::string internal_name; 
        std::string name; // name that is shown on the UI
        int version_major = 0;
        int version_minor = 0;
        int version_patch = 0;

        // Internal stuff!
        ImGuiStyle _style; // NOLINT
        std::array<std::string, static_cast<size_t>(Icon::Count)> _icon_paths; // NOLINT
    };
    
    class ThemeManager {
        public:
            // can throw std::runtime_error if it fails to reload the theme cache or apply a theme 
            ThemeManager(
                Renderer::IRenderer& renderer, 
                Common::FS::VFS::VFS& vfs, 
                const std::string& theme
            );

            ~ThemeManager();

            /** 
                @param internal_name If it is org.cattle_engine.editor_themes.system_default it will use the default system theme (light or dark).
                This does not apply the theme until the end of the ImGui frame (if no_wait_until_end_frame is false)

                @param no_wait_until_end_frame Set to true only if ImGui::BeginFrame() was not called before calling this
            */
            Status SetTheme(const std::string& internal_name, bool no_wait_until_end_frame = false);
            // finds themes inside /config/user_themes
            Status ReloadThemeCache();
            const std::vector<Theme>& GetThemes();
            Theme GetCurrentTheme();
            
            // returns the renderer's native texture, for use with imgui
            void* GetNativeIconTexture(Icon icon);

            // called at the end of the frame 
            void ApplyTheme();
            bool WantsThemeApply() const;
        private:
            bool mNeedThemeApply = false;

            Renderer::IRenderer& mRenderer;
            Common::FS::VFS::VFS& mVFS;
            std::vector<Theme> mListedThemes;
            std::array<Renderer::Texture*, static_cast<size_t>(Icon::Count)> mIcons{};
            Theme mCurrentTheme;
    };
}