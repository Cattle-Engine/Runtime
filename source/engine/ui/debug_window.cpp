#include "engine/ui/debug_window.hpp"

#include <cinttypes>
#include <cstring>
#include <format>
#include <string>

#include <SDL3/SDL.h>

#include "engine/assets/fonts.hpp"
#include "engine/audio/audio_resource_manager.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/input/mouse.hpp"
#include "engine/instance.hpp"
#include "engine/memory/allocator.hpp"
#include "engine/memory/stats.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/settings.hpp"
#include "engine/ui/utils.hpp"
#include "engine/version.hpp"
#include "engine/common/window.hpp"

#include "imgui.h"
#include "imgui_stdlib.h"
#include <git_version.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

constexpr std::array<const char*, static_cast<size_t>(CE::Common::Window::WindowMode::Count)> WindowModeNames = {
    "Fullscreen",
    "Borderless",
    "Windowed"
};

std::string FormatBytes(std::size_t bytes) {
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;

    double value = static_cast<double>(bytes);

    if (value >= GB) {
        return std::format("{:.2f} GB", value / GB);
    }

    if (value >= MB) {
        return std::format("{:.2f} MB", value / MB);
    }

    if (value >= KB) {
        return std::format("{:.2f} KB", value / KB);
    }

    return std::format("{} B", bytes);
}

namespace CE::UI {
    DebugWindow::DebugWindow(Renderer::IRenderer& renderer, Renderer::Resources::TextureManager& texman,
                             Renderer::Resources::ShaderManager& shaderman, Assets::Fonts::FontManager& fontman,
                             GameInfo& gameinfo, Settings::SettingsManager& settings,
                             Audio::Resources::AudioManager* audioman, Input::Keyboard& keyboard,
                             Instance& instance, Input::Mouse& mouse)
        : mRenderer(renderer), mTextureManager(texman), mShaderManager(shaderman), mFontManager(fontman),
          mGameInfo(gameinfo), mSettings(settings), mAudioManager(audioman), mKeyboard(keyboard),
          mInstance(instance), mMouse(mouse), mMemoryTrackingEnabled(Memory::IsTrackingEnabled()) {}

    void DebugWindow::SetOpen(bool open) {
        gOpen = open;
    }

    bool DebugWindow::IsOpen() const {
        return gOpen;
    }

    void DebugWindow::UpdateFreeCam(float deltaTime) {
        if (mKeyboard.IsKeyDown(Input::KeyboardKeys::KEY_LEFT_CONTROL) &&
            mKeyboard.IsKeyDown(Input::KeyboardKeys::KEY_LEFT_SHIFT)) {
            gFreeCam.enabled = false;
            mMouse.LockCursor(false);
            mMouse.SetCursorVisibility(Input::MouseVisibility::Shown);
            return;
        }

        if (!gFreeCam.enabled)
            return;

        auto* cam = mRenderer.GetCamera3D();
        if (!cam)
            return;

        mMouse.LockCursor(true);
        mMouse.SetCursorVisibility(Input::MouseVisibility::Hidden);

        cam->useTarget = false;

        cam->rotation.y -= mMouse.GetDeltaX() * gFreeCam.sensitivity;
        cam->rotation.x += mMouse.GetDeltaY() * gFreeCam.sensitivity;

        constexpr float kPitchLimit = glm::radians(89.0f);
        cam->rotation.x = glm::clamp(cam->rotation.x, -kPitchLimit, kPitchLimit);

        const glm::mat4 cameraRotation = glm::mat4_cast(glm::quat(cam->rotation));
        const glm::vec3 forward = glm::normalize(glm::vec3(cameraRotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
        const glm::vec3 right = -glm::normalize(glm::vec3(cameraRotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

        float speed = gFreeCam.speed * deltaTime;

        if (mKeyboard.IsKeyDown(Input::KeyboardKeys::KEY_W))
            cam->position += forward * speed;

        if (mKeyboard.IsKeyDown(Input::KeyboardKeys::KEY_S))
            cam->position -= forward * speed;

        if (mKeyboard.IsKeyDown(Input::KeyboardKeys::KEY_A))
            cam->position += right * speed;

        if (mKeyboard.IsKeyDown(Input::KeyboardKeys::KEY_D))
            cam->position -= right * speed;

        if (mKeyboard.IsKeyDown(Input::KeyboardKeys::KEY_SPACE))
            cam->position -= worldUp * speed;

        if (mKeyboard.IsKeyDown(Input::KeyboardKeys::KEY_LEFT_SHIFT))
            cam->position += worldUp * speed;
    }

    void DebugWindow::DrawInstanceTab() {
        ImGui::Text("InstanceID: %i", mInstance.GetInstanceID());

        if (ImGui::Button("Quit instance")) {
            mInstance.Exit();
        }

        Utils::SpaceSep();

        ImGui::Text("State");

        if (ImGui::InputText("Change the state", &mGameState, ImGuiInputTextFlags_EnterReturnsTrue)) {
            mInstance.SetGameState(mGameState);
        }
        ImGui::Text("Press enter to apply");
        ImGui::Text("Current state: %s", mInstance.GetGameState().c_str());

        Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Gameinfo")) {
            ImGui::Text("Game name: %s", mGameInfo.gameNameString.c_str());
            ImGui::Text("Game version: %s", mGameInfo.gameVersionString.c_str());
            ImGui::Text("Window title: %s", mGameInfo.windowTitle.c_str());
            ImGui::Text("Window size: %i x %i", mGameInfo.windowWidth, mGameInfo.windowHeight);
            ImGui::Text("VSync: %s", mGameInfo.enableVSync ? "Enabled" : "Disabled");
            ImGui::Text(
                "Window Mode: %s",
                WindowModeNames[static_cast<size_t>(mGameInfo.windowMode)]
            );
            ImGui::Text("Resizable Window: %s", mGameInfo.resizableWindow ? "Yes" : "No");
        }

        Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Engine info")) {
            ImGui::Text("Build string: %s", CE::Version::GetBuildString().c_str());

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to copy to clipboard");

                if (ImGui::IsMouseClicked(0)) {
                    SDL_SetClipboardText(CE::Version::GetBuildString().c_str());
                }
            }

            ImGui::Text("Version string: %s", CE::Version::engineVersionString);
            ImGui::Text("Version number: %d.%d.%d", CE::Version::engineVersionMajor, CE::Version::engineVersionMinor,
                        CE::Version::engineVersionPatch);

            Utils::SpaceSep();

            ImGui::Text("Git infomation: ");
            ImGui::Text("Branch: %s", CE_GIT_BRANCH);
            ImGui::Text("Commit hash: %s", CE_GIT_HASH_FULL);
            ImGui::Text("Commit dirty: %s", CE_GIT_ISDIRTY);
            ImGui::Text("Tags: %s", CE_GIT_TAGS);
        }
    }

    void DebugWindow::DrawInputTab() {
        ImGui::Text("Keyboard");
        ImGui::Spacing();
        ImGui::Text("Currently held keys: %s", mKeyboard.GetPressedKeysString().c_str());

        Utils::SpaceSep();

        ImGui::Text("Mouse");
        ImGui::Spacing();
        ImGui::Text("Mouse posX: %i", mMouse.GetX());
        ImGui::Text("Mouse posY: %i", mMouse.GetY());
        ImGui::Text("Mouse delta posX: %i", mMouse.GetDeltaX());
        ImGui::Text("Mouse delta posY: %i", mMouse.GetDeltaY());
        ImGui::Spacing();
        ImGui::Text("Mouse wheelX: %i", mMouse.GetWheelX());
        ImGui::Text("Mouse wheelY: %i", mMouse.GetWheelY());
    }

    void DebugWindow::DrawPerformanceTab() {

        ImGui::Text("Performance");
        ImGui::Spacing();

        ImGui::Text("FPS: %d", mInstance.GetFPS());
        ImGui::Text("Frame Time (ms): %.3f", mInstance.GetFrameTime());
        ImGui::Text("Delta Time (s): %.6f", mInstance.GetDeltaTime());

        gFpsHistory[static_cast<size_t>(gFpsHistoryOffset)] = static_cast<float>(mInstance.GetFPS());
        gFpsHistoryOffset = (gFpsHistoryOffset + 1) % static_cast<int>(gFpsHistory.size());

        ImGui::PlotLines("FPS History", gFpsHistory.data(), static_cast<int>(gFpsHistory.size()), 0, nullptr, 0.0f,
                         300.0f, ImVec2(0, 80));

        Utils::SpaceSep();

        ImGui::Text("Memory");

        auto& memory = Memory::GetStats();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Note: Total allocations/deallocations include every C++ new/delete.");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        if (ImGui::BeginTable("MemoryTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Total Allocations");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", memory.allocations.load());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Total Deallocations");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", memory.deallocations.load());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Current Alive");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%zu", memory.aliveAllocations.load());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Peak Live");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%zu", memory.peakAliveAllocations.load());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Memory Allocated");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", FormatBytes(memory.bytesAllocated.load()).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Memory Freed");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", FormatBytes(memory.bytesFreed.load()).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Currently Used");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", FormatBytes(memory.currentBytes.load()).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Peak Memory");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", FormatBytes(memory.peakBytes.load()).c_str());

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Checkbox("Enable Memory Tracking", &mMemoryTrackingEnabled);
        CE::Memory::EnableTracking(mMemoryTrackingEnabled);
    }

    void DebugWindow::DrawSettingsTab() {
        auto& s = mSettings.Settings;
        auto& state = gSettingsState;

        ImGui::Text("Window");
        ImGui::Spacing();

        ImGui::InputInt("Width", &s.windowWidth);
        ImGui::InputInt("Height", &s.windowHeight);

        int window_mode = static_cast<int>(s.windowMode);
        if (ImGui::Combo(
            "Window Mode",
            &window_mode,
            WindowModeNames.data(),
            static_cast<int>(WindowModeNames.size()))
        ) {
            s.windowMode = static_cast<Common::Window::WindowMode>(window_mode);
        }

        ImGui::Checkbox("VSync", &s.enableVSync);

        Utils::SpaceSep();

        ImGui::Text("Performance");
        ImGui::Spacing();

        ImGui::SliderInt("Max FPS", &s.maxFPS, 5, 240);
        ImGui::Text("Note: This is ignored if VSync is on and\nFPS is locked to display refresh rate");

        Utils::SpaceSep();

        ImGui::Text("Renderer");
        ImGui::Spacing();

        if (!state.synced) {
            std::strncpy(state.rendererBuffer.data(), s.rendererName.c_str(), state.rendererBuffer.size() - 1);
            state.rendererBuffer[state.rendererBuffer.size() - 1] = '\0';
            state.synced = true;
        }

        ImGui::PushID(&mSettings);
        ImGui::InputText("Renderer", state.rendererBuffer.data(), state.rendererBuffer.size());
        ImGui::PopID();

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            s.rendererName = state.rendererBuffer.data();
        }

        ImGui::Text("Supported renderers: Metal, DX12, Vulkan, Software");
        ImGui::Text("Note: To change renderer you need to close engine and reopen.");

        Utils::SpaceSep();

        ImGui::Text("Audio");
        ImGui::Spacing();

        bool audio_dirty = false;
        audio_dirty |= ImGui::SliderFloat("Master Volume", &s.masterVolume, 0.0f, 1.0f, "%.2f");
        audio_dirty |= ImGui::SliderFloat("Music Volume", &s.musicVolume, 0.0f, 1.0f, "%.2f");
        audio_dirty |= ImGui::SliderFloat("SFX Volume", &s.sfxVolume, 0.0f, 1.0f, "%.2f");
        if (audio_dirty && mAudioManager) {
            mAudioManager->SetMasterVolume(s.masterVolume);
            mAudioManager->SetMusicVolume(s.musicVolume);
            mAudioManager->SetSFXVolume(s.sfxVolume);
        }

        Utils::SpaceSep();

        ImGui::Text("Settings path: %s", mSettings.GetSettingPath().c_str());

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Click to copy to clipboard");

            if (ImGui::IsMouseClicked(0)) {
                SDL_SetClipboardText(mSettings.GetSettingPath().c_str());
            }
        }

        if (ImGui::Button("Save & apply")) {
            mSettings.FlushSettings();
            mSettings.ReloadSettings();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reload from disk")) {
            mSettings.ReloadSettings();

            std::strncpy(state.rendererBuffer.data(), s.rendererName.c_str(), state.rendererBuffer.size() - 1);
            state.rendererBuffer[state.rendererBuffer.size() - 1] = '\0';
            state.synced = true;
        }
    }

    void DebugWindow::DrawRendererTab() {
        ImGui::Text("Current renderer: %s", mSettings.Settings.rendererName.c_str());

        Utils::SpaceSep();

        ImGui::Checkbox("Enable FreeCam", &gFreeCam.enabled);
        ImGui::Text("To exit freecam press: CTR + Shift ");
        ImGui::SliderFloat("Move Speed", &gFreeCam.speed, 0.1f, 50.0f);
        ImGui::SliderFloat("Mouse Sensitivity", &gFreeCam.sensitivity, 0.001f, 0.1f, "%.4f");

        if (ImGui::Button("Reset FreeCam")) {
            gFreeCam.sensitivity = 0.02f;
            gFreeCam.speed = 5.0f;
        }

        Utils::SpaceSep();

        Renderer::Camera2D* camera = mRenderer.GetCamera();

        ImGui::Text("Camera2D");
        ImGui::Text("Position: %f X, %f Y", camera->x, camera->y);
        ImGui::Text("Zoom: %f", camera->zoom);

        ImGui::Text("Edit Camera");
        ImGui::InputFloat("X", &camera->x);
        ImGui::InputFloat("Y", &camera->y);
        ImGui::SliderFloat("Zoom", &camera->zoom, 0.1f, 10.0f, "%.2f");

        // Clamp zoom so I don't break stuff
        if (camera->zoom < 0.01f)
            camera->zoom = 0.01f;
        if (ImGui::Button("Reset 2D Camera")) {
            camera->x = 0.0f;
            camera->y = 0.0f;
            camera->zoom = 1.0f;
        }

        Utils::SpaceSep();

        ImGui::Text("Camera3D");
        auto camera3 = mRenderer.GetCamera3D();

        ImGui::InputFloat3("Position", &camera3->position.x);
        ImGui::InputFloat3("Rotation", &camera3->rotation.x);

        ImGui::Checkbox("Use Target", &camera3->useTarget);
        ImGui::SliderFloat("FOV", &camera3->fov, 0.1f, glm::radians(120.0f));
        ImGui::InputFloat("Near", &camera3->nearClip);
        ImGui::InputFloat("Far", &camera3->farClip);

        ImGui::Combo("Projection", (int*)&camera3->projection, "Perspective\0Orthographic\0");

        ImGui::InputFloat("Ortho Size", &camera3->orthoSize);

        if (ImGui::Button("Reset 3D Camera")) {
            camera3->position = glm::vec3(0.0f);
            camera3->rotation = glm::vec3(0.0f);

            camera3->fov = glm::radians(60.0f);
            camera3->nearClip = 0.1f;
            camera3->farClip = 1000.0f;

            camera3->useTarget = false;
            camera3->projection = Renderer::Camera3D::ProjectionMode::Perspective;

            camera3->orthoSize = 10.0f;
        }

        Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Geometry")) {
            ImGui::Text("Vertex Count: %d", mRenderer.Debug_GetVertCount());
            ImGui::Text("Texture Vertex Count: %d", mRenderer.Debug_GetTexVertCount());
            ImGui::Text("Index Count: %d", mRenderer.Debug_GetIndexCount());
            ImGui::Text("Texture Index Count: %d", mRenderer.Debug_GetTexIndexCount());
            ImGui::Text("Note: When using the software renderer,\nthese are meant to be empty.");
        }

        CE::UI::Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Textures")) {
            ImGui::Text("Total loaded: %zu", mTextureManager.GetLoadedTextureCount());
            ImGui::Text("No error: %zu", mTextureManager.GetValidTextureCount());
            ImGui::Text("Errors: %zu", mTextureManager.GetErrorTextureCount());
            ImGui::Text("Pending Unload: %zu", mTextureManager.GetPendingUnloadCount());
        }

        CE::UI::Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Shaders")) {
            ImGui::Text("Total loaded: %zu", mShaderManager.Debug_LoadedShadersCount());
            ImGui::Text("No error: %d", mShaderManager.Debug_LoadedShadersNoError());
            ImGui::Text("Errors: %d", mShaderManager.Debug_LoadedShadersError());
            ImGui::Text("Bound shader: %" PRIu64, mShaderManager.Debug_GetBoundShaderID().id);

            auto shaders = mShaderManager.Debug_GetShaders();
            if (ImGui::TreeNode("Shader List")) {
                for (const auto& shader : shaders) {
                    ImGui::PushID(shader.id);
                    if (ImGui::TreeNode("Shader")) {
                        ImGui::Text("Compiled: %s", shader.isCompiled ? "Yes" : "No");
                        ImGui::Text("Error: %s", shader.isErrorShader ? "Yes" : "No");
                        ImGui::Text("Bound: %s", shader.isBound ? "Yes" : "No");
                        ImGui::Text("Default Vertex: %s", shader.usesDefaultVertex ? "Yes" : "No");
                        ImGui::Text("Default Fragment: %s", shader.usesDefaultFragment ? "Yes" : "No");
                        ImGui::Text("Vertex Path: %s",
                                    shader.vertexPath.empty() ? "<default>" : shader.vertexPath.c_str());
                        ImGui::Text("Fragment Path: %s",
                                    shader.fragmentPath.empty() ? "<default>" : shader.fragmentPath.c_str());
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }

        CE::UI::Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Fonts")) {

            auto defaultFont = mFontManager.Debug_GetDefaultFontName();
            ImGui::Text("Default Font: %s", defaultFont.c_str());

            auto atlases = mFontManager.Debug_GetAtlases();
            ImGui::Text("Atlases: %zu", atlases.size());

            CE::UI::Utils::SpaceSep();

            ImGui::Text("Atlas Viewer");

            ImGui::InputText("Family", gAtlasFamilyBuf.data(), gAtlasFamilyBuf.size());
            ImGui::InputInt("Size", &gAtlasSizeBuf);

            if (gAtlasSizeBuf < 1)
                gAtlasSizeBuf = 1;

            auto* tex = mFontManager.Debug_GetAtlasTex(gAtlasFamilyBuf.data(), gAtlasSizeBuf);

            if (tex) {
                ImGui::Text("Atlas Preview:");
                void* nativeTexture = mRenderer.GetNativeTextureHandle(tex);
                if (nativeTexture) {
                    ImGui::Image((ImTextureID)(intptr_t)nativeTexture, ImVec2(256, 256));
                } else {
                    ImGui::TextDisabled("Atlas texture is not available for ImGui preview");
                }
            } else {
                ImGui::TextDisabled("No atlas found");
            }

            CE::UI::Utils::SpaceSep();

            if (ImGui::TreeNode("Atlas List")) {

                for (const auto& a : atlases) {

                    ImGui::PushID(a.key.c_str());

                    if (ImGui::TreeNode(a.key.c_str())) {

                        ImGui::Text("Family: %s", a.familyName.c_str());
                        ImGui::Text("Size: %d", a.fontSize);
                        ImGui::Text("Glyphs: %zu", a.glyphCount);

                        ImGui::Text("Atlas: %dx%d", a.atlasWidth, a.atlasHeight);
                        ImGui::Text("Pen: %d, %d", a.penX, a.penY);
                        ImGui::Text("RowH: %d", a.rowH);

                        ImGui::Text("Texture: %s", a.hasTexture ? "Yes" : "No");
                        ImGui::Text("Dirty: %s", a.dirty ? "Yes" : "No");

                        ImGui::Text("Memory: %.2f KB", a.estimatedMemoryBytes / 1024.0f);

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
        }
    }

    void DebugWindow::DrawAudioTab() {
        ImGui::Text("Audio");
        ImGui::Spacing();

        auto& s = mSettings.Settings;

        bool dirty = false;
        dirty |= ImGui::SliderFloat("Master Volume", &s.masterVolume, 0.0f, 1.0f, "%.2f");
        dirty |= ImGui::SliderFloat("Music Volume", &s.musicVolume, 0.0f, 1.0f, "%.2f");
        dirty |= ImGui::SliderFloat("SFX Volume", &s.sfxVolume, 0.0f, 1.0f, "%.2f");

        if (dirty && mAudioManager) {
            mAudioManager->SetMasterVolume(s.masterVolume);
            mAudioManager->SetMusicVolume(s.musicVolume);
            mAudioManager->SetSFXVolume(s.sfxVolume);
        }

        CE::UI::Utils::SpaceSep();

        if (!mAudioManager) {
            ImGui::TextDisabled("Audio system not available");
            return;
        }

        ImGui::Text("Cached Clips: %zu", mAudioManager->Debug_CachedClipsCount());

        const auto snapshot = mAudioManager->Debug_PlayingSoundsSnapshot();
        ImGui::Text("Playing Handles: %zu", snapshot.size());

        if (ImGui::BeginTable("AudioPlayingTable", 7,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Handle");
            ImGui::TableSetupColumn("Clip");
            ImGui::TableSetupColumn("Bus");
            ImGui::TableSetupColumn("Vol");
            ImGui::TableSetupColumn("Playing");
            ImGui::TableSetupColumn("FX");
            ImGui::TableSetupColumn("Actions");
            ImGui::TableHeadersRow();

            for (const auto& row : snapshot) {
                ImGui::PushID(static_cast<int>(row.Handle.id));
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", row.Handle.id);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.Label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(row.Bus.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", row.Volume);
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(row.IsPlaying ? "Yes" : "No");
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%zu", row.EffectCount);

                ImGui::TableSetColumnIndex(6);
                if (ImGui::SmallButton("Play")) {
                    mAudioManager->PlaySound(row.Handle);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Pause")) {
                    mAudioManager->PauseSound(row.Handle);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Resume")) {
                    mAudioManager->ResumeSound(row.Handle);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Stop")) {
                    mAudioManager->StopSound(row.Handle);
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void DebugWindow::Draw() {
        if (!gOpen) {
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(487, 386), ImGuiCond_FirstUseEver);
        ImGui::Begin("Cattle Debug");

        if (ImGui::BeginTabBar("DebugTabs")) {
            if (ImGui::BeginTabItem("Instance")) {
                DrawInstanceTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Input")) {
                DrawInputTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings")) {
                DrawSettingsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Performance")) {
                DrawPerformanceTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Audio")) {
                DrawAudioTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Renderer")) {
                DrawRendererTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        UpdateFreeCam(mInstance.GetDeltaTime());
        ImGui::End();
    }
} // namespace CE::UI
