#include <cstring>
#include <cinttypes>

#include "imgui/imgui.h"
#include "third_party/imgui_stdlib.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <git_version.hpp>
#include "engine/ui/debug_window.hpp"
#include "engine/ui/utils.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/assets/skybox_manager.hpp"
#include "engine/rendering/resources/shader_manager.hpp"
#include "engine/version.hpp"
#include "engine/assets/fonts.hpp"
#include "engine/input/mouse.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/instance.hpp"
#include "engine/settings.hpp"
#include "engine/assets/audio.hpp"

namespace CE::UI {
    void DebugWindow::SetOpen(bool open) {
        gOpen = open;
    }

    bool DebugWindow::IsOpen() const {
        return gOpen;
    }

    void DebugWindow::UpdateFreeCam(
        Renderer::IRenderer& renderer,
        Input::Keyboard& keyboard,
        Input::Mouse& mouse,
        float deltaTime
    ) {
        if (keyboard.IsKeyDown(Input::KeyboardKeys::KEY_LEFT_CONTROL) &&
            keyboard.IsKeyDown(Input::KeyboardKeys::KEY_LEFT_SHIFT)) {
            gFreeCam.enabled = false;
            mouse.LockCursor(false);
            mouse.SetCursorVisibility(Input::MouseVisibility::Shown);
            return;
        }

        if (!gFreeCam.enabled)
            return;

        auto* cam = renderer.GetCamera3D();
        if (!cam)
            return;

        mouse.LockCursor(true);
        mouse.SetCursorVisibility(Input::MouseVisibility::Hidden);

        cam->useTarget = false;

        cam->rotation.y -= mouse.GetDeltaX() * gFreeCam.sensitivity;
        cam->rotation.x -= mouse.GetDeltaY() * gFreeCam.sensitivity;

        constexpr float kPitchLimit = glm::radians(89.0f);
        cam->rotation.x = glm::clamp(
            cam->rotation.x,
            -kPitchLimit,
            kPitchLimit
        );

        const glm::mat4 cameraRotation = glm::mat4_cast(glm::quat(cam->rotation));
        const glm::vec3 forward = glm::normalize(glm::vec3(cameraRotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
        const glm::vec3 right = -glm::normalize(glm::vec3(cameraRotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

        float speed = gFreeCam.speed * deltaTime;

        if (keyboard.IsKeyDown(Input::KeyboardKeys::KEY_W))
            cam->position += forward * speed;

        if (keyboard.IsKeyDown(Input::KeyboardKeys::KEY_S))
            cam->position -= forward * speed;
            
        if (keyboard.IsKeyDown(Input::KeyboardKeys::KEY_A))
            cam->position += right * speed;

        if (keyboard.IsKeyDown(Input::KeyboardKeys::KEY_D))
            cam->position -= right * speed;

        if (keyboard.IsKeyDown(Input::KeyboardKeys::KEY_SPACE))
            cam->position += worldUp * speed;

        if (keyboard.IsKeyDown(Input::KeyboardKeys::KEY_LEFT_SHIFT))
            cam->position -= worldUp * speed;
    }

    void DebugWindow::DrawInstanceTab(GameInfo& gameinfo, Instance& instance) {
        static std::string game_state = "";
        ImGui::Text("InstanceID: %i", instance.GetInstanceID());

        if (ImGui::Button("Quit instance")) {
            instance.Exit();   
        }

        Utils::SpaceSep();

        ImGui::Text("State");

        if (ImGui::InputText("Change the state", &game_state, ImGuiInputTextFlags_EnterReturnsTrue)) {
            instance.SetGameState(game_state);
        }
        ImGui::Text("Press enter to apply");
        ImGui::Text("Current state: %s", instance.GetGameState().c_str());

        Utils::SpaceSep();
        
        if (ImGui::CollapsingHeader("Gameinfo")) {
            ImGui::Text("Game name: %s", gameinfo.gameNameString.c_str());
            ImGui::Text("Game version: %s", gameinfo.gameVersionString.c_str());
            ImGui::Text("Window title: %s", gameinfo.windowTitle.c_str());
            ImGui::Text("Window size: %i x %i", gameinfo.windowWidth, gameinfo.windowHeight);
            ImGui::Text("VSync: %s", gameinfo.enableVSync ? "Enabled" : "Disabled");
            ImGui::Text("Fullscreen: %s", gameinfo.fullscreen ? "Yes" : "No");
            ImGui::Text("Resizable Window: %s", gameinfo.resizableWindow ? "Yes" : "No");
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
            ImGui::Text("Version number: %d.%d.%d", CE::Version::engineVersionMajor, CE::Version::engineVersionMinor, CE::Version::engineVersionPatch);

            Utils::SpaceSep();

            ImGui::Text("Git infomation: ");
            ImGui::Text("Branch: %s", CE_GIT_BRANCH);
            ImGui::Text("Commit hash: %s", CE_GIT_HASH_FULL);
            ImGui::Text("Commit dirty: %s", CE_GIT_ISDIRTY);
            ImGui::Text("Tags: %s", CE_GIT_TAGS);
        }
    }

    void DebugWindow::DrawInputTab(Input::Keyboard& kbmanger, Input::Mouse& msmanager) {
        ImGui::Text("Keyboard");
        ImGui::Spacing();
        ImGui::Text("Currently held keys: %s", kbmanger.GetPressedKeysString().c_str());

        Utils::SpaceSep();

        ImGui::Text("Mouse");
        ImGui::Spacing();
        ImGui::Text("Mouse posX: %i", msmanager.GetX());
        ImGui::Text("Mouse posY: %i", msmanager.GetY());
        ImGui::Text("Mouse delta posX: %i", msmanager.GetDeltaX());
        ImGui::Text("Mouse delta posY: %i", msmanager.GetDeltaY());
        ImGui::Spacing();
        ImGui::Text("Mouse wheelX: %i", msmanager.GetWheelX());
        ImGui::Text("Mouse wheelY: %i", msmanager.GetWheelY());
    }

    void DebugWindow::DrawPerformanceTab(
        CE::Renderer::IRenderer& renderer,
        CE::Renderer::Resources::TextureManager& texman,
        CE::Renderer::Resources::ShaderManager& shaderman,
        CE::Assets::Skyboxes::SkyBoxManager& skyboxman,
        const CE::Settings::SettingsManager& settings,
        int fps,
        float deltaTime,
        float frameTime
    ) {
        (void)renderer;
        (void)texman;
        (void)shaderman;
        (void)skyboxman;
        (void)settings;

        ImGui::Text("Performance");
        ImGui::Spacing();

        ImGui::Text("FPS: %d", fps);
        ImGui::Text("Frame Time (ms): %.3f", frameTime);
        ImGui::Text("Delta Time (s): %.6f", deltaTime);

        gFpsHistory[static_cast<size_t>(gFpsHistoryOffset)] = static_cast<float>(fps);
        gFpsHistoryOffset = (gFpsHistoryOffset + 1) % static_cast<int>(gFpsHistory.size());

        ImGui::PlotLines(
            "FPS History",
            gFpsHistory.data(),
            static_cast<int>(gFpsHistory.size()),
            0,
            nullptr,
            0.0f,
            300.0f,
            ImVec2(0, 80)
        );
    }

    void DebugWindow::DrawSettingsTab(CE::Settings::SettingsManager& settings, CE::Assets::Audio::AudioManager* audioman) {
        auto& s = settings.Settings;
        auto& state = gSettingsState;

        ImGui::Text("Window");
        ImGui::Spacing();

        ImGui::InputInt("Width", &s.windowWidth);
        ImGui::InputInt("Height", &s.windowHeight);

        ImGui::Checkbox("Fullscreen", &s.fullscreen);
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

        ImGui::PushID(&settings);
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
        if (audio_dirty && audioman) {
            audioman->SetMasterVolume(s.masterVolume);
            audioman->SetMusicVolume(s.musicVolume);
            audioman->SetSFXVolume(s.sfxVolume);
        }

        Utils::SpaceSep();

        ImGui::Text("Settings path: %s", settings.GetSettingPath().c_str());

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Click to copy to clipboard");

            if (ImGui::IsMouseClicked(0)) {
                SDL_SetClipboardText(settings.GetSettingPath().c_str());
            }
        }

        if (ImGui::Button("Save & apply")) {
            settings.FlushSettings();
            settings.ReloadSettings();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reload from disk")) {
            settings.ReloadSettings();

            std::strncpy(state.rendererBuffer.data(), s.rendererName.c_str(), state.rendererBuffer.size() - 1);
            state.rendererBuffer[state.rendererBuffer.size() - 1] = '\0';
            state.synced = true;
        }
    }

    void DebugWindow::DrawRendererTab(
        CE::Renderer::IRenderer& renderer,
        const Settings::SettingsManager& settings,
        Renderer::Resources::TextureManager& texman,
        CE::Renderer::Resources::ShaderManager& shaderman,
        Assets::Skyboxes::SkyBoxManager& skyboxman,
        Assets::Fonts::FontManager& fontman
    ) 
    {
        ImGui::Text("Current renderer: %s", settings.Settings.rendererName.c_str());
        
        Utils::SpaceSep();
        
        ImGui::Checkbox("Enable FreeCam", &gFreeCam.enabled);
        ImGui::Text("To exit freecam press: CTR + Shift ");
        ImGui::SliderFloat("Move Speed", &gFreeCam.speed, 0.1f, 50.0f);
        ImGui::SliderFloat(
            "Mouse Sensitivity",
            &gFreeCam.sensitivity,
            0.001f,
            0.1f,
            "%.4f"
        );

        if (ImGui::Button("Reset FreeCam")) {
            gFreeCam.sensitivity = 0.02f;
            gFreeCam.speed = 5.0f;
        }

        Utils::SpaceSep();

        Renderer::Camera2D* camera = renderer.GetCamera();

        ImGui::Text("Camera2D");
        ImGui::Text("Position: %f X, %f Y", camera->x, camera->y);
        ImGui::Text("Zoom: %f", camera->zoom);

        ImGui::Text("Edit Camera");
        ImGui::InputFloat("X", &camera->x);
        ImGui::InputFloat("Y", &camera->y);
        ImGui::SliderFloat("Zoom", &camera->zoom, 0.1f, 10.0f, "%.2f");

        // Clamp zoom so I don't break stuff
        if (camera->zoom < 0.01f) camera->zoom = 0.01f;
        if (ImGui::Button("Reset 2D Camera")) {
            camera->x = 0.0f;
            camera->y = 0.0f;
            camera->zoom = 1.0f;
        }

        Utils::SpaceSep();

        ImGui::Text("Camera3D");
        auto camera3 = renderer.GetCamera3D();

        ImGui::InputFloat3("Position", &camera3->position.x);
        ImGui::InputFloat3("Rotation", &camera3->rotation.x);

        ImGui::Checkbox("Use Target", &camera3->useTarget);
        ImGui::SliderFloat("FOV", &camera3->fov, 0.1f, glm::radians(120.0f));
        ImGui::InputFloat("Near", &camera3->nearClip);
        ImGui::InputFloat("Far", &camera3->farClip);

        ImGui::Combo("Projection",
            (int*)&camera3->projection,
            "Perspective\0Orthographic\0");

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

        CE::UI::Utils::SpaceSep();

        if (ImGui::CollapsingHeader("SkyBoxes")) {
            ImGui::Text("Active Skybox: %s", skyboxman.Debug_GetBoundSkyBoxName().c_str());
            ImGui::Text("Loaded: %d", skyboxman.Debug_LoadedSkyBoxesCount());
            ImGui::Text("Valid: %d", skyboxman.Debug_LoadedSkyBoxesNoError());
            ImGui::Text("Errored: %d", skyboxman.Debug_LoadedSkyBoxesError());

            ImGui::SeparatorText("Create / Load");
            ImGui::InputText("Skybox Name", &gSkyBoxState.name);
            ImGui::InputText("Front", &gSkyBoxState.front);
            ImGui::InputText("Back", &gSkyBoxState.back);
            ImGui::InputText("Left", &gSkyBoxState.left);
            ImGui::InputText("Right", &gSkyBoxState.right);
            ImGui::InputText("Top", &gSkyBoxState.top);
            ImGui::InputText("Bottom", &gSkyBoxState.bottom);

            if (ImGui::Button("Load Skybox")) {
                skyboxman.Load(
                    gSkyBoxState.front,
                    gSkyBoxState.back,
                    gSkyBoxState.left,
                    gSkyBoxState.right,
                    gSkyBoxState.top,
                    gSkyBoxState.bottom,
                    gSkyBoxState.name
                );
            }
            ImGui::SameLine();
            if (ImGui::Button("Set Active")) {
                skyboxman.Set(gSkyBoxState.name);
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Active")) {
                skyboxman.Set("");
            }

            CE::UI::Utils::SpaceSep();

            auto skyboxes = skyboxman.Debug_GetSkyBoxes();
            if (ImGui::TreeNode("Loaded Skyboxes")) {
                for (const auto& skybox : skyboxes) {
                    ImGui::PushID(skybox.name.c_str());
                    if (ImGui::TreeNode(skybox.name.c_str())) {
                        ImGui::Text("Active: %s", skybox.isActive ? "Yes" : "No");
                        ImGui::Text("Error Skybox: %s", skybox.isErrorSkyBox ? "Yes" : "No");
                        ImGui::Text("Front: %s", skybox.frontPath.c_str());
                        ImGui::Text("Back: %s", skybox.backPath.c_str());
                        ImGui::Text("Left: %s", skybox.leftPath.c_str());
                        ImGui::Text("Right: %s", skybox.rightPath.c_str());

                        if (ImGui::Button("Set")) {
                            skyboxman.Set(skybox.name.c_str());
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Unload")) {
                            skyboxman.Unload(skybox.name.c_str());
                            ImGui::TreePop();
                            ImGui::PopID();
                            continue;
                        }

                        auto previewFace = [&](const char* label, const std::shared_ptr<CE::Renderer::Texture>& face) {
                            ImGui::Text("%s", label);
                            if (face) {
                                void* nativeTexture = renderer.GetNativeTextureHandle(face.get());
                                if (nativeTexture) {
                                    ImGui::Image((ImTextureID)(intptr_t)nativeTexture, ImVec2(96, 96));
                                } else {
                                    ImGui::TextDisabled("No native preview available");
                                }
                            } else {
                                ImGui::TextDisabled("Missing face");
                            }
                        };

                        previewFace("Front", skybox.cubeMap.front);
                        previewFace("Back", skybox.cubeMap.back);
                        previewFace("Left", skybox.cubeMap.left);
                        previewFace("Right", skybox.cubeMap.right);

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }

        Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Geometry")) {
            ImGui::Text("Vertex Count: %d", renderer.Debug_GetVertCount());
            ImGui::Text("Texture Vertex Count: %d", renderer.Debug_GetTexVertCount());
            ImGui::Text("Index Count: %d", renderer.Debug_GetIndexCount());
            ImGui::Text("Texture Index Count: %d", renderer.Debug_GetTexIndexCount());
            ImGui::Text("Note: When using the software renderer,\nthese are meant to be empty.");
        }

        CE::UI::Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Textures")) {
            ImGui::Text("Total loaded: %zu", texman.GetLoadedTextureCount());
            ImGui::Text("No error: %zu", texman.GetValidTextureCount());
            ImGui::Text("Errors: %zu", texman.GetErrorTextureCount());
            ImGui::Text("Pending Unload: %zu", texman.GetPendingUnloadCount());
        }

        CE::UI::Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Shaders")) {
            ImGui::Text("Total loaded: %zu", shaderman.Debug_LoadedShadersCount());
            ImGui::Text("No error: %d", shaderman.Debug_LoadedShadersNoError());
            ImGui::Text("Errors: %d", shaderman.Debug_LoadedShadersError());
            ImGui::Text("Bound shader: %" PRIu64, shaderman.Debug_GetBoundShaderID().id);

            auto shaders = shaderman.Debug_GetShaders();
            if (ImGui::TreeNode("Shader List")) {
                for (const auto& shader : shaders) {
                    ImGui::PushID(shader.id);
                    if (ImGui::TreeNode("Shader")) {
                        ImGui::Text("Compiled: %s", shader.isCompiled ? "Yes" : "No");
                        ImGui::Text("Error: %s", shader.isErrorShader ? "Yes" : "No");
                        ImGui::Text("Bound: %s", shader.isBound ? "Yes" : "No");
                        ImGui::Text("Default Vertex: %s", shader.usesDefaultVertex ? "Yes" : "No");
                        ImGui::Text("Default Fragment: %s", shader.usesDefaultFragment ? "Yes" : "No");
                        ImGui::Text("Vertex Path: %s", shader.vertexPath.empty() ? "<default>" : shader.vertexPath.c_str());
                        ImGui::Text("Fragment Path: %s", shader.fragmentPath.empty() ? "<default>" : shader.fragmentPath.c_str());
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }

        CE::UI::Utils::SpaceSep();

        if (ImGui::CollapsingHeader("Fonts")) {

            auto defaultFont = fontman.Debug_GetDefaultFontName();
            ImGui::Text("Default Font: %s", defaultFont.c_str());

            auto atlases = fontman.Debug_GetAtlases();
            ImGui::Text("Atlases: %zu", atlases.size());

            CE::UI::Utils::SpaceSep();

            ImGui::Text("Atlas Viewer");

            ImGui::InputText("Family", gAtlasFamilyBuf.data(), gAtlasFamilyBuf.size());
            ImGui::InputInt("Size", &gAtlasSizeBuf);

            if (gAtlasSizeBuf < 1) gAtlasSizeBuf = 1;

            auto* tex = fontman.Debug_GetAtlasTex(gAtlasFamilyBuf.data(), gAtlasSizeBuf);

            if (tex) {
                ImGui::Text("Atlas Preview:");
                void* nativeTexture = renderer.GetNativeTextureHandle(tex);
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

                        ImGui::Text("Memory: %.2f KB",
                            a.estimatedMemoryBytes / 1024.0f);

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
        }
    }

    void DebugWindow::DrawAudioTab(CE::Assets::Audio::AudioManager* audioman, CE::Settings::SettingsManager& settings) {
        ImGui::Text("Audio");
        ImGui::Spacing();

        auto& s = settings.Settings;

        bool dirty = false;
        dirty |= ImGui::SliderFloat("Master Volume", &s.masterVolume, 0.0f, 1.0f, "%.2f");
        dirty |= ImGui::SliderFloat("Music Volume", &s.musicVolume, 0.0f, 1.0f, "%.2f");
        dirty |= ImGui::SliderFloat("SFX Volume", &s.sfxVolume, 0.0f, 1.0f, "%.2f");

        if (dirty && audioman) {
            audioman->SetMasterVolume(s.masterVolume);
            audioman->SetMusicVolume(s.musicVolume);
            audioman->SetSFXVolume(s.sfxVolume);
        }

        CE::UI::Utils::SpaceSep();

        if (!audioman) {
            ImGui::TextDisabled("Audio system not available");
            return;
        }

        ImGui::Text("Cached Clips: %zu", audioman->Debug_CachedClipsCount());

        const auto snapshot = audioman->Debug_PlayingSoundsSnapshot();
        ImGui::Text("Playing Handles: %zu", snapshot.size());

        if (ImGui::BeginTable("AudioPlayingTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Handle");
            ImGui::TableSetupColumn("Clip");
            ImGui::TableSetupColumn("Bus");
            ImGui::TableSetupColumn("Vol");
            ImGui::TableSetupColumn("Playing");
            ImGui::TableSetupColumn("FX");
            ImGui::TableSetupColumn("Actions");
            ImGui::TableHeadersRow();

            for (const auto& row : snapshot) {
                ImGui::PushID(static_cast<int>(row.Handle));
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", row.Handle);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(row.ClipName.c_str());
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
                    audioman->PlaySound(row.Handle);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Pause")) {
                    audioman->PauseSound(row.Handle);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Resume")) {
                    audioman->ResumeSound(row.Handle);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Stop")) {
                    audioman->StopSound(row.Handle);
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }
    
    void DebugWindow::Draw(
        CE::Renderer::IRenderer& renderer,
        CE::Renderer::Resources::TextureManager& texman,
        CE::Renderer::Resources::ShaderManager& shaderman,
        CE::Assets::Skyboxes::SkyBoxManager& skyboxman,
        CE::Assets::Fonts::FontManager& fontman,
        CE::GameInfo& gameinfo,
        CE::Settings::SettingsManager& settings,
        CE::Assets::Audio::AudioManager* audioman,
        Input::Keyboard& kbmanger,
        CE::Instance& instance,
        Input::Mouse& msmanager,
        int fps,
        float deltaTime,
        float frameTime
    ) {
        if (!gOpen) {
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(487, 386), ImGuiCond_FirstUseEver);
        ImGui::Begin("Cattle Debug");

        if (ImGui::BeginTabBar("DebugTabs")) {
            if (ImGui::BeginTabItem("Instance")) {
                DrawInstanceTab(gameinfo, instance);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Input")) {
                DrawInputTab(kbmanger, msmanager);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings")) {
                DrawSettingsTab(settings, audioman);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Performance")) {
                DrawPerformanceTab(renderer, texman, shaderman, skyboxman, settings, fps, deltaTime, frameTime);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Audio")) {
                DrawAudioTab(audioman, settings);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Renderer")) {
                DrawRendererTab(renderer, settings, texman, shaderman, skyboxman, fontman);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        this->UpdateFreeCam(renderer, kbmanger, msmanager, instance.GetDeltaTime());
        ImGui::End();
    }

    void DrawDebugUI(
        CE::Renderer::IRenderer& renderer,
        CE::Renderer::Resources::TextureManager& texman,
        CE::Renderer::Resources::ShaderManager& shaderman,
        CE::Assets::Skyboxes::SkyBoxManager& skyboxman,
        CE::Assets::Fonts::FontManager& fontman,
        CE::GameInfo& gameinfo,
        CE::Settings::SettingsManager& settings,
        CE::Assets::Audio::AudioManager* audioman,
        Input::Keyboard& kbmanger,
        CE::Instance& instance,
        Input::Mouse& msmanager,
        int fps,
        float deltaTime,
        float frameTime
    ) {
        static DebugWindow window;
        window.Draw(
            renderer,
            texman,
            shaderman,
            skyboxman,
            fontman,
            gameinfo,
            settings,
            audioman,
            kbmanger,
            instance,
            msmanager,
            fps,
            deltaTime,
            frameTime
        );
    }
}
