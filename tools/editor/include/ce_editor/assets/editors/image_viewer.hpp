#pragma once

#include "ce_editor/ui/i_window.hpp"
#include <imgui.h>

#include "ce_runtime/rendering/resources/texture_manager.hpp"
#include "ce_editor/assets/asset_editor_manager.hpp"

namespace CE::Editor::Assets {
    class ImageViewer : public IAssetEditor {
        public:
            using IAssetEditor::IAssetEditor;
            ~ImageViewer() override;

            std::string GetWindowName() override;
            void Draw() override;

            void Open(std::string_view vfs_path) override;
            void Close() override;
        private:
            Renderer::Resources::TextureHandle mTexture;   
            std::string mImagePath;
            float mZoom = 1.0f;
            ImVec2 mPan = ImVec2(0.0f, 0.0f);
            bool mShouldResizeToImage = false;
            bool mFitToWindow = true;
    };
}