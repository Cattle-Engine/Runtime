#include <algorithm>
#include <cmath>

#include <imgui.h>

#include "ce_common/tracelog.hpp"

#include "ce_editor/editor.hpp"
#include "ce_editor/assets/editors/image_viewer.hpp"

namespace CE::Editor::Assets {
    std::string ImageViewer::GetWindowName() {
        if (mImagePath.empty()) return "ImageViewer - No image loaded";
        return "ImageViewer - " + mImagePath;
    }

    ImageViewer::~ImageViewer() {
        Close();
    }

    void ImageViewer::Open(std::string_view vfs_path) {
        auto handle = mEditorInstance.GetTextureManager().Load(std::string(vfs_path));

        if (!handle) {
            CE_LOG(CECommon::LogLevel::Error, "[ImageViewer] Failed to open image or create texture on file: {}", vfs_path);
            return;
        }

        mTexture = std::move(handle);
        mImagePath = vfs_path;
        mShouldResizeToImage = true;
    }

    void ImageViewer::Close() {
        if (mTexture) {
            mEditorInstance.GetTextureManager().Unload(mTexture);
        }
        mImagePath = "";
    }

    void ImageViewer::Draw() {
        if (mShouldResizeToImage && mTexture) {
            constexpr float toolbar_height = 45.0f;
            constexpr float window_padding = 16.0f;
            constexpr float screen_margin = 40.0f;

            const ImVec2 display_size = ImGui::GetIO().DisplaySize;

            const float image_width = static_cast<float>(mTexture.GetTextureWidth());
            const float image_height = static_cast<float>(mTexture.GetTextureHeight());

            // Maximum available window dimensions.
            const float max_width = display_size.x - screen_margin;
            const float max_height = display_size.y - screen_margin;

            // Account for toolbar and window padding.
            const float available_width = max_width - window_padding;
            const float available_height = max_height - toolbar_height - window_padding;

            // Calculate scale needed to fit the image on screen.
            const float scale = std::min({
                1.0f,
                available_width / image_width,
                available_height / image_height
            });

            // Calculate final window dimensions.
            const ImVec2 window_size(
                (image_width * scale) + window_padding,
                (image_height * scale) + toolbar_height + window_padding
            );

            ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);

            mShouldResizeToImage = false;
        }

        if (!ImGui::Begin(GetWindowName().c_str(), &mWindowOpen)) {
            ImGui::End();
            return;
        }

        // Toolbar
        if (ImGui::Button("Fit")) {
            mFitToWindow = true;
            mZoom = 1.0f;
            mPan = ImVec2(0.0f, 0.0f);
        }

        ImGui::SameLine();

        if (ImGui::Button("100%")) {
            mFitToWindow = false;
            mZoom = 1.0f;
            mPan = ImVec2(0.0f, 0.0f);
        }

        ImGui::SameLine();

        if (ImGui::Button("-")) {
            mFitToWindow = false;
            mZoom /= 1.25f;
        }

        ImGui::SameLine();

        if (ImGui::Button("+")) {
            mFitToWindow = false;
            mZoom *= 1.25f;
        }

        ImGui::SameLine();

        ImGui::Text("%.0f%%", mZoom * 100.0f);

        ImGui::SameLine();

        if (mTexture) {
            ImGui::TextDisabled(
                "%d x %d",
                mTexture.GetTextureWidth(),
                mTexture.GetTextureHeight()
            );
        }

        ImGui::Separator();

        // Viewport
        const ImVec2 viewport_size = ImGui::GetContentRegionAvail();

        ImGui::BeginChild(
            "ImageViewport",
            viewport_size,
            ImGuiChildFlags_None,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        const ImVec2 viewport_pos = ImGui::GetWindowPos();
        const ImVec2 available = ImGui::GetContentRegionAvail();

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Background
        draw_list->AddRectFilled(
            viewport_pos,
            ImVec2(viewport_pos.x + available.x, viewport_pos.y + available.y),
            IM_COL32(35, 35, 35, 255)
        );

        if (!mTexture) {
            const char* message = "No image loaded";

            const ImVec2 text_size = ImGui::CalcTextSize(message);

            draw_list->AddText(
                ImVec2(
                    viewport_pos.x + ((available.x - text_size.x) * 0.5f),
                    viewport_pos.y + ((available.y - text_size.y) * 0.5f)
                ),
                IM_COL32(150, 150, 150, 255),
                message
            );

            ImGui::EndChild();
            ImGui::End();
            return;
        }

        const float image_width = static_cast<float>(mTexture.GetTextureWidth());
        const float image_height = static_cast<float>(mTexture.GetTextureHeight());

        if (image_width <= 0.0f || image_height <= 0.0f ||
            available.x <= 0.0f || available.y <= 0.0f) {
            ImGui::EndChild();
            ImGui::End();
            return;
        }

        // Calculate fit scale
        const float fit_scale = std::min(
            available.x / image_width,
            available.y / image_height
        );

        // Calculate final image dimensions
        const float scale = fit_scale * mZoom;

        const ImVec2 image_size(
            image_width * scale,
            image_height * scale
        );

        // Viewport centre
        const ImVec2 centre(
            viewport_pos.x + (available.x * 0.5f),
            viewport_pos.y + (available.y * 0.5f)
        );

        // Mouse interaction
        const bool hovered = ImGui::IsWindowHovered();

        if (hovered) {
            // Zoom towards mouse position
            const float wheel = ImGui::GetIO().MouseWheel;

            if (wheel != 0.0f) {
                const float old_zoom = mZoom;

                mZoom = std::clamp(
                    mZoom * std::pow(1.15f, wheel),
                    0.05f,
                    32.0f
                );

                const float zoom_ratio = mZoom / old_zoom;

                const ImVec2 mouse = ImGui::GetMousePos();

                const ImVec2 anchor(
                    mouse.x - centre.x,
                    mouse.y - centre.y
                );

                mPan.x = anchor.x + ((mPan.x - anchor.x) * zoom_ratio);
                mPan.y = anchor.y + ((mPan.y - anchor.y) * zoom_ratio);

                mFitToWindow = false;
            }

            // Middle mouse button to pan
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                const ImVec2 delta = ImGui::GetIO().MouseDelta;

                mPan.x += delta.x;
                mPan.y += delta.y;

                mFitToWindow = false;
            }
        }

        // Recalculate after interaction
        const float final_scale = fit_scale * mZoom;

        const ImVec2 final_size(
            image_width * final_scale,
            image_height * final_scale
        );

        // Checkerboard background
        constexpr float kCheckerSize = 16.0f;

        const ImU32 checker_a = IM_COL32(55, 55, 55, 255);
        const ImU32 checker_b = IM_COL32(70, 70, 70, 255);

        for (float y = 0; y < available.y; y += kCheckerSize) {
            for (float x = 0; x < available.x; x += kCheckerSize) {
                const int column = static_cast<int>(x / kCheckerSize);
                const int row = static_cast<int>(y / kCheckerSize);

                const ImU32 color = (column + row) % 2 == 0
                    ? checker_a
                    : checker_b;

                draw_list->AddRectFilled(
                    ImVec2(viewport_pos.x + x, viewport_pos.y + y),
                    ImVec2(
                        viewport_pos.x + std::min(x + kCheckerSize, available.x),
                        viewport_pos.y + std::min(y + kCheckerSize, available.y)
                    ),
                    color
                );
            }
        }

        // Position image in viewport
        const ImVec2 image_pos(
            centre.x - (final_size.x * 0.5f) + mPan.x,
            centre.y - (final_size.y * 0.5f) + mPan.y
        );

        ImGui::SetCursorScreenPos(image_pos);

        ImGui::Image(
            static_cast<ImTextureRef>(mEditorInstance.GetRenderer().GetNativeTextureHandle(mTexture.GetTexture())),
            final_size
        );

        ImGui::EndChild();
        ImGui::End();
    }
}