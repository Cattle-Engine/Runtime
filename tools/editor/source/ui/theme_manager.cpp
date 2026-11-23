#include <array>
#include <algorithm>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <SDL3/SDL_video.h>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

#include "ce_runtime/common/tracelog.hpp"
#include "ce_editor/ui/theme_manager.hpp"
#include "ce_runtime/common/fs/file_provider.hpp"
#include "ce_runtime/common/fs/vfs.hpp"
#include "ce_runtime/rendering/renderer.hpp"
#include "ce_common/tracelog.hpp"

namespace CE::Editor::UI {
    namespace {
        constexpr std::string_view kSystemDefaultThemeName = "org.cattle_engine.editor_themes.system_default";

        bool SystemPrefersDark() {
            SDL_SystemTheme theme = SDL_GetSystemTheme();
            return theme == SDL_SYSTEM_THEME_DARK;
        }

        template<typename T>
        void ReadOptional(const YAML::Node& node, const char* key, T& value) {
            if (node[key]) {
                value = node[key].as<T>();
            }
        }

        // Flag fields are read as raw integers.
        template<typename T>
        void ReadOptionalFlags(const YAML::Node& node, const char* key, T& value) {
            if (node[key]) {
                value = static_cast<T>(node[key].as<int>());
            }
        }

        void ReadOptionalVec2(const YAML::Node& node, const char* key, ImVec2& value) {
            if (!node[key]) return;

            const auto& arr = node[key];

            if (!arr.IsSequence() || arr.size() != 2) {
                throw std::runtime_error(std::format(
                    "Expected '{}' to be an array of 2 values", key
                ));
            }

            value.x = arr[0].as<float>();
            value.y = arr[1].as<float>();
        }

        void ReadOptionalVec4(const YAML::Node& node, const char* key, ImVec4& value) {
            if (!node[key]) return;

            const auto& arr = node[key];

            if (!arr.IsSequence() || arr.size() != 4) {
                throw std::runtime_error(std::format(
                    "Expected '{}' to be an array of 4 values", key
                ));
            }

            value.x = arr[0].as<float>();
            value.y = arr[1].as<float>();
            value.z = arr[2].as<float>();
            value.w = arr[3].as<float>();
        }

        void ReadOptionalDir(const YAML::Node& node, const char* key, ImGuiDir& value) {
            if (!node[key]) return;

            const auto str = node[key].as<std::string>();

            if (str == "none")       value = ImGuiDir_None;
            else if (str == "left")  value = ImGuiDir_Left;
            else if (str == "right") value = ImGuiDir_Right;
            else if (str == "up")    value = ImGuiDir_Up;
            else if (str == "down")  value = ImGuiDir_Down;
            else {
                throw std::runtime_error(std::format(
                    "Invalid direction '{}' for '{}': expected none, left, right, up or down", str, key
                ));
            }
        }

        void ReadStyle(const YAML::Node& node, ImGuiStyle& style) {
            if (!node) return;

            // Fonts (1.92+)
#if IMGUI_VERSION_NUM >= 19200
            ReadOptional(node, "font_size_base", style.FontSizeBase);
            ReadOptional(node, "font_scale_main", style.FontScaleMain);
            // FontScaleDpi is intentionally skipped: backends set it per-monitor.
#endif

            // General
            ReadOptional(node, "alpha", style.Alpha);
            ReadOptional(node, "disabled_alpha", style.DisabledAlpha);

            // Window
            ReadOptionalVec2(node, "window_padding", style.WindowPadding);
            ReadOptional(node, "window_rounding", style.WindowRounding);
            ReadOptional(node, "window_border_size", style.WindowBorderSize);
#if IMGUI_VERSION_NUM >= 19200
            ReadOptional(node, "window_border_hover_padding", style.WindowBorderHoverPadding);
#endif
            ReadOptionalVec2(node, "window_min_size", style.WindowMinSize);
            ReadOptionalVec2(node, "window_title_align", style.WindowTitleAlign);
            ReadOptionalDir(node, "window_menu_button_position", style.WindowMenuButtonPosition);

            // Child / Popup
            ReadOptional(node, "child_rounding", style.ChildRounding);
            ReadOptional(node, "child_border_size", style.ChildBorderSize);
            ReadOptional(node, "popup_rounding", style.PopupRounding);
            ReadOptional(node, "popup_border_size", style.PopupBorderSize);

            // Frame
            ReadOptionalVec2(node, "frame_padding", style.FramePadding);
            ReadOptional(node, "frame_rounding", style.FrameRounding);
            ReadOptional(node, "frame_border_size", style.FrameBorderSize);

            // Spacing
            ReadOptionalVec2(node, "item_spacing", style.ItemSpacing);
            ReadOptionalVec2(node, "item_inner_spacing", style.ItemInnerSpacing);
            ReadOptionalVec2(node, "cell_padding", style.CellPadding);
            ReadOptionalVec2(node, "touch_extra_padding", style.TouchExtraPadding);
            ReadOptional(node, "indent_spacing", style.IndentSpacing);
            ReadOptional(node, "columns_min_spacing", style.ColumnsMinSpacing);

            // Scrollbar
            ReadOptional(node, "scrollbar_size", style.ScrollbarSize);
            ReadOptional(node, "scrollbar_rounding", style.ScrollbarRounding);
#if IMGUI_VERSION_NUM >= 19200
            ReadOptional(node, "scrollbar_padding", style.ScrollbarPadding);
#endif

            // Grab / Slider
            ReadOptional(node, "grab_min_size", style.GrabMinSize);
            ReadOptional(node, "grab_rounding", style.GrabRounding);
            ReadOptional(node, "log_slider_deadzone", style.LogSliderDeadzone);

            // Images
#if IMGUI_VERSION_NUM >= 19190
            ReadOptional(node, "image_border_size", style.ImageBorderSize);
#endif

            // Tabs
            ReadOptional(node, "tab_rounding", style.TabRounding);
            ReadOptional(node, "tab_border_size", style.TabBorderSize);
#if IMGUI_VERSION_NUM >= 19200
            ReadOptional(node, "tab_min_width_base", style.TabMinWidthBase);
            ReadOptional(node, "tab_min_width_shrink", style.TabMinWidthShrink);
            ReadOptional(node, "tab_close_button_min_width_selected", style.TabCloseButtonMinWidthSelected);
            ReadOptional(node, "tab_close_button_min_width_unselected", style.TabCloseButtonMinWidthUnselected);
#endif
            ReadOptional(node, "tab_bar_border_size", style.TabBarBorderSize);
#if IMGUI_VERSION_NUM >= 19090
            ReadOptional(node, "tab_bar_overline_size", style.TabBarOverlineSize);
#endif

            // Tables
            ReadOptional(node, "table_angled_headers_angle", style.TableAngledHeadersAngle);
            ReadOptionalVec2(node, "table_angled_headers_text_align", style.TableAngledHeadersTextAlign);

            // Tree lines
#if IMGUI_VERSION_NUM >= 19150
            ReadOptionalFlags(node, "tree_lines_flags", style.TreeLinesFlags);
            ReadOptional(node, "tree_lines_size", style.TreeLinesSize);
            ReadOptional(node, "tree_lines_rounding", style.TreeLinesRounding);
#endif

            // Widgets
            ReadOptionalDir(node, "color_button_position", style.ColorButtonPosition);
            ReadOptionalVec2(node, "button_text_align", style.ButtonTextAlign);
            ReadOptionalVec2(node, "selectable_text_align", style.SelectableTextAlign);

            // Separator text
            ReadOptional(node, "separator_text_border_size", style.SeparatorTextBorderSize);
            ReadOptionalVec2(node, "separator_text_align", style.SeparatorTextAlign);
            ReadOptionalVec2(node, "separator_text_padding", style.SeparatorTextPadding);

            // Docking
#ifdef IMGUI_HAS_DOCK
            ReadOptional(node, "docking_separator_size", style.DockingSeparatorSize);
#endif

            // Misc
            ReadOptionalVec2(node, "display_window_padding", style.DisplayWindowPadding);
            ReadOptionalVec2(node, "display_safe_area_padding", style.DisplaySafeAreaPadding);
            ReadOptional(node, "mouse_cursor_scale", style.MouseCursorScale);

            // Rendering
            ReadOptional(node, "anti_aliased_lines", style.AntiAliasedLines);
            ReadOptional(node, "anti_aliased_lines_use_tex", style.AntiAliasedLinesUseTex);
            ReadOptional(node, "anti_aliased_fill", style.AntiAliasedFill);
            ReadOptional(node, "curve_tessellation_tol", style.CurveTessellationTol);
            ReadOptional(node, "circle_tessellation_max_error", style.CircleTessellationMaxError);

            // Tooltips / hover behavior
            ReadOptional(node, "hover_stationary_delay", style.HoverStationaryDelay);
            ReadOptional(node, "hover_delay_short", style.HoverDelayShort);
            ReadOptional(node, "hover_delay_normal", style.HoverDelayNormal);
            ReadOptionalFlags(node, "hover_flags_for_tooltip_mouse", style.HoverFlagsForTooltipMouse);
            ReadOptionalFlags(node, "hover_flags_for_tooltip_nav", style.HoverFlagsForTooltipNav);
        }

        void ReadColors(const YAML::Node& node, ImGuiStyle& style) {
            if (!node) return;

            for (int i = 0; i < ImGuiCol_COUNT; ++i) {
                const auto color = static_cast<ImGuiCol>(i);
                const char* color_name = ImGui::GetStyleColorName(color);

                if (!node[color_name]) continue;

                const auto& arr = node[color_name];

                if (!arr.IsSequence() || arr.size() != 4) {
                    throw std::runtime_error(std::format(
                        "Invalid color '{}': expected an array of 4 values",
                        color_name
                    ));
                }

                style.Colors[i] = ImVec4(
                    arr[0].as<float>(),
                    arr[1].as<float>(),
                    arr[2].as<float>(),
                    arr[3].as<float>()
                );
            }
        }

        Theme ParseTheme(const YAML::Node& root) {
            if (!root["theme"] || !root["theme"].IsMap()) {
                throw std::runtime_error("Missing or invalid 'theme' section");
            }

            const auto node = root["theme"];

            if (!node["internal_name"] || !node["name"] || !node["version"]) {
                throw std::runtime_error("Theme is missing required metadata");
            }

            Theme theme;

            theme.internal_name = node["internal_name"].as<std::string>();
            theme.name = node["name"].as<std::string>();

            if (theme.internal_name.empty() || theme.name.empty()) {
                throw std::runtime_error("Theme name and internal_name cannot be empty");
            }

            const auto version = node["version"];

            if (!version.IsMap()) {
                throw std::runtime_error("Invalid theme version");
            }

            theme.version_major = version["major"].as<int>();
            theme.version_minor = version["minor"].as<int>();
            theme.version_patch = version["patch"].as<int>();

            if (theme.version_major < 0 ||
                theme.version_minor < 0 ||
                theme.version_patch < 0) {
                throw std::runtime_error("Theme version components cannot be negative");
            }

            // Parse style.
            ReadStyle(node["style"], theme._style);

            // Parse colors.
            ReadColors(node["colors"], theme._style);

            // Parse icons.
            if (node["icons"]) {
                const auto icons = node["icons"];

                constexpr std::array<const char*, static_cast<size_t>(Icon::Count)> kIconNames = {
                    "undo",
                    "redo"
                };

                for (size_t i = 0; i < kIconNames.size(); ++i) {
                    if (icons[kIconNames[i]]) {
                        theme._icon_paths[i] = icons[kIconNames[i]].as<std::string>();
                    }
                }
            }

            return theme;
        }
    }

    ThemeManager::ThemeManager(
        Renderer::IRenderer& renderer,
        Common::FS::VFS::VFS& vfs,
        const std::string& theme
    ) : mRenderer(renderer), mVFS(vfs) {
        auto cache_result = ReloadThemeCache();
        if (!cache_result) throw std::runtime_error(cache_result.GetError().message);

        // Applies immediatelty
        auto load_theme_result = SetTheme(theme, true);
        if (!load_theme_result) throw std::runtime_error(load_theme_result.GetError().message);
    }

    Status ThemeManager::ReloadThemeCache() {
        mListedThemes.clear();

        // Loads every theme found in sub-directories of `base_dir`.
        // Each theme is expected at <base_dir>/<name>/theme.yaml.
        auto load_themes_from = [this](const std::string& base_dir, std::string_view source) {
            const auto dir_list = mVFS.ListDirectory(base_dir);

            for (const auto& dir_content : dir_list) {
                if (dir_content.type == Common::FS::VFS::DirectoryContent::Type::File) continue;

                const std::string theme_path = std::format("{}/{}/theme.yaml", base_dir, dir_content.name);

                if (!mVFS.FileExists(theme_path)) {
                    CE_LOG(
                        LogLevel::Warn,
                        "[ThemeManager] {} theme directory '{}' exists but has no theme.yaml at '{}'.",
                        source, dir_content.name, theme_path
                    );
                    continue;
                }

                auto theme_file = mVFS.OpenFile(theme_path);
                if (!theme_file) {
                    CE_LOG(LogLevel::Warn, "[ThemeManager] Failed to open theme file: {}", theme_path);
                    continue;
                }

                auto theme_file_data = theme_file->ReadToString();
                if (!theme_file_data) {
                    CE_LOG(LogLevel::Warn, "[ThemeManager] Failed to read theme file: '{}' to a string", theme_path);
                    continue;
                }

                try {
                    YAML::Node in = YAML::Load(*theme_file_data);
                    Theme theme = ParseTheme(in);

                    // Prevent duplicate identifiers (earlier entries win, so inbuilt
                    // themes can't be shadowed by user themes).
                    const auto duplicate = std::find_if(
                        mListedThemes.begin(),
                        mListedThemes.end(),
                        [&theme](const Theme& existing) {
                            return existing.internal_name == theme.internal_name;
                        }
                    );

                    if (duplicate != mListedThemes.end()) {
                        CE_LOG(
                            LogLevel::Warn,
                            "[ThemeManager] Duplicate theme identifier '{}'. Skipping {} theme '{}'.",
                            theme.internal_name, source, dir_content.name
                        );
                        continue;
                    }

                    mListedThemes.push_back(std::move(theme));

                } catch (const std::exception& e) {
                    CE_LOG(
                        LogLevel::Warn,
                        "[ThemeManager] Failed to parse theme '{}': {}",
                        theme_path, e.what()
                    );
                }
            }
        };

        // Inbuilt themes (required)
        {
            if (!mVFS.DirExists("/themes")) {
                return Error {
                    .message = "Inbuilt theme directory was not found! Is the installation corrupt?",
                    .context = "ThemeManager::ReloadThemeCache()",
                    .code = std::make_error_code(std::errc::no_such_file_or_directory),
                };
            }

            if (mVFS.ListDirectory("/themes").empty()) {
                return Error {
                    .message = "Inbuilt theme directory was empty, is the installation corrupt?",
                    .context = "ThemeManager::ReloadThemeCache()",
                };
            }

            load_themes_from("/themes", "Inbuilt");
        }

        // User themes (optional)
        if (mVFS.DirExists("/config/user_themes")) {
            load_themes_from("/config/user_themes", "User");
        }

        return {};
    }

    Status ThemeManager::SetTheme(const std::string& internal_name, bool no_wait_until_end_frame) {
        const Theme* found = nullptr;
        Theme system_theme;

        if (internal_name == kSystemDefaultThemeName) {
            system_theme.internal_name = std::string(kSystemDefaultThemeName);
            system_theme.name = "System Default";

            if (SystemPrefersDark()) ImGui::StyleColorsDark(&system_theme._style);
            else                     ImGui::StyleColorsLight(&system_theme._style);

            system_theme._icon_paths[static_cast<size_t>(Icon::Undo)] = "/textures/undo_icon.png";
            system_theme._icon_paths[static_cast<size_t>(Icon::Redo)] = "/textures/redo_icon.png";

            found = &system_theme;
        } else {
            for (const auto& theme : mListedThemes) {
                if (theme.internal_name == internal_name) {
                    found = &theme;
                    break;
                }
            }
        }

        if (!found) {
            return Error {
                .message = "Theme '" + internal_name + "' was not found",
                .context = "ThemeManager::SetTheme",
            };
        }

        mCurrentTheme = *found;

        if (no_wait_until_end_frame) {
            ApplyTheme();
        } else {
            mNeedThemeApply = true;
        }

        return {};
    }

    const std::vector<Theme>& ThemeManager::GetThemes() {
        return mListedThemes;
    }

    Theme ThemeManager::GetCurrentTheme() {
        return mCurrentTheme;
    }

    void* ThemeManager::GetNativeIconTexture(Icon icon) {
        const auto index = static_cast<size_t>(icon);

        if (index >= mIcons.size() || !mIcons[index]) {
            return mRenderer.GetNativeTextureHandle(mRenderer.GetErrorTexture());
        }

        return mRenderer.GetNativeTextureHandle(mIcons[index]);
    }

    bool ThemeManager::WantsThemeApply() const {
        return mNeedThemeApply;
    }

    void ThemeManager::ApplyTheme() {
        mNeedThemeApply = false;

        // Style
        {
            ImGuiStyle& style = ImGui::GetStyle();

            // Keep runtime/backend-owned scaling state instead of overwriting it
            // with the theme's defaults.
#if IMGUI_VERSION_NUM >= 19200
            const float font_scale_dpi = style.FontScaleDpi;
            const float main_scale = style._MainScale;
#endif

            style = mCurrentTheme._style;

#if IMGUI_VERSION_NUM >= 19200
            style.FontScaleDpi = font_scale_dpi;
            style._MainScale = main_scale;
#endif
        }

        // Icons
        for (size_t i = 0; i < mIcons.size(); ++i) {
            if (mIcons[i]) {
                mRenderer.UnloadTex(mIcons[i]);
                mIcons[i] = nullptr;
            }

            const std::string& path = mCurrentTheme._icon_paths[i];
            if (path.empty()) continue;

            if (!mVFS.FileExists(path)) {
                CE_LOG(LogLevel::Warn, "[ThemeManager] Icon '{}' for theme '{}' was not found.",
                    path, mCurrentTheme.internal_name);
                continue;
            }

            mIcons[i] = mRenderer.LoadTex(path.c_str(), Renderer::TextureFilter::Nearest);
            if (!mIcons[i]) {
                CE_LOG(LogLevel::Warn, "[ThemeManager] Renderer failed to create texture for '{}'.", path);
            }
        }
    }

    ThemeManager::~ThemeManager() {
        for (auto* texture : mIcons) {
            if (texture) {
                mRenderer.UnloadTex(texture);
            }
        }
    }
}