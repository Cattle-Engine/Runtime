#include "ce_runtime/settings.hpp"

#include <algorithm>
#include <filesystem>
#include <format>

#include "ce_runtime/common/fs/ini.hpp"
#include "ce_runtime/common/tracelog.hpp"
#include "ce_runtime/common/window.hpp"
#include "ce_runtime/platforms.hpp"

namespace CE::Settings {
    SettingsManager::SettingsManager(std::string package_id, uint64_t instance_id, std::optional<GameInfo> gameinfo)
        : mGamePackageID(std::move(package_id)), mGameInfo(std::move(gameinfo)), mInstanceID(instance_id) {
        Internal_ReloadSettings();
    }

    bool SettingsManager::Internal_ReloadSettings() {
        const std::string config_path = std::format(
            "{}/{}",
            Platforms::GetConfigPath(mGamePackageID.c_str()), "settings.cfg"
        );

        // Resolve defaults and limits from GameInfo if present, otherwise built-in fallbacks.
        struct Resolved {
            int windowHeight = 720;
            int windowWidth = 1280;
            int maxFPS = 60;
            bool enableVSync = true;
            std::string rendererName = "Vulkan";
            Common::Window::WindowMode windowMode = static_cast<Common::Window::WindowMode>(2); // Windowed
            int64_t minWindowHeight = 1;
            int64_t maxWindowHeight = std::numeric_limits<int>::max();
            int64_t minWindowWidth = 1;
            int64_t maxWindowWidth = std::numeric_limits<int>::max();
        } d;

        if (mGameInfo) {
            const GameInfo& gi = *mGameInfo;
            d.windowHeight = gi.windowHeight;
            d.windowWidth = gi.windowWidth;
            d.maxFPS = gi.maxFPS;
            d.enableVSync = gi.enableVSync;
            d.rendererName = gi.rendererName;
            d.windowMode = gi.windowMode;
            d.minWindowHeight = gi.minWindowHeight;
            d.maxWindowHeight = gi.maxWindowHeight;
            d.minWindowWidth = gi.minWindowWidth;
            d.maxWindowWidth = gi.maxWindowWidth;
        }

        auto apply_defaults = [&]() {
            Settings.windowHeight = d.windowHeight;
            Settings.windowWidth = d.windowWidth;
            Settings.maxFPS = d.maxFPS;
            Settings.enableVSync = d.enableVSync;
            Settings.rendererName = d.rendererName;
            Settings.windowMode = d.windowMode;
            Settings.masterVolume = 1.0f;
            Settings.musicVolume = 1.0f;
            Settings.sfxVolume = 1.0f;
            Settings.audioDeviceName = "DefaultAudioDevice";
        };

        // first launch creates the configuration file.
        if (!std::filesystem::exists(config_path)) {
            CE_LOG(
                LogLevel::Info,
                "[Settings Manager {}] No settings.cfg found. Creating default configuration.",
                mInstanceID
            );

            std::error_code ec;
            std::filesystem::create_directories(std::filesystem::path(config_path).parent_path(), ec);

            if (ec) {
                CE_LOG(
                    LogLevel::Error,
                    "[Settings Manager {}] Failed to create config directory: {}",
                    mInstanceID,
                    ec.message()
                );
                return false;
            }

            apply_defaults();
            mIniFile = CE::Ini::IniFile{};

            FlushSettings();
            return true;
        }

        if (!CE::Ini::load_file(config_path, mIniFile, &mParseError)) {
            CE_LOG(
                LogLevel::Error,
                "[Settings Manager {}] Setting parser error\n Line: {}, Column: {}, Message: {}",
                mInstanceID,
                mParseError.line,
                mParseError.column,
                mParseError.message
            );

            apply_defaults();
            return false;
        }

        Settings.windowHeight = static_cast<int>(
            std::clamp<int64_t>(
                mIniFile.get_int("graphics", "window_height", d.windowHeight),
                d.minWindowHeight,
                d.maxWindowHeight
            )
        );

        Settings.windowWidth = static_cast<int>(
            std::clamp<int64_t>(
                mIniFile.get_int("graphics", "window_width", d.windowWidth),
                d.minWindowWidth,
                d.maxWindowWidth
            )
        );

        Settings.maxFPS = mIniFile.get_int("graphics", "max_fps", d.maxFPS);
        Settings.enableVSync = mIniFile.get_bool("graphics", "vsync", d.enableVSync);
        Settings.rendererName = mIniFile.get_string("graphics", "renderer", d.rendererName);

        int window_mode = mIniFile.get_int("graphics", "window_mode", static_cast<int>(d.windowMode));

        Settings.masterVolume = mIniFile.get_float("audio", "master_volume", 1.0f);
        Settings.musicVolume = mIniFile.get_float("audio", "music_volume", 1.0f);
        Settings.sfxVolume = mIniFile.get_float("audio", "sfx_volume", 1.0f);
        Settings.audioDeviceName = mIniFile.get_string("audio", "audio_device", "DefaultAudioDevice");

        if (window_mode >= 0 && window_mode <= 2) {
            Settings.windowMode = static_cast<Common::Window::WindowMode>(window_mode);
        } else {
            CE_LOG(LogLevel::Error, "[Settings] Window_Mode is out of bounds, using default");
            CE_LOG(LogLevel::Debug, "0 = Fullscreen, 1 = Borderless, 2 = Windowed");
            Settings.windowMode = d.windowMode;
        }

        return true;
    }

    void SettingsManager::OnReload(std::function<void()> callback){
        mOnReloadedCallback = std::move(callback);
    }

    bool SettingsManager::ReloadSettings() {
        CE_LOG(LogLevel::Debug, "[Settings Manager {}] To change renderer you need to reload engine!", mInstanceID);
        bool reloaded = Internal_ReloadSettings();
        if (reloaded && mOnReloadedCallback) {
            mOnReloadedCallback();
        }
        return reloaded;
    }

    void SettingsManager::FlushSettings() {
        mIniFile.set_int("graphics", "window_height", Settings.windowHeight);
        mIniFile.set_int("graphics", "window_width", Settings.windowWidth);
        mIniFile.set_int("graphics", "max_fps", Settings.maxFPS);
        mIniFile.set_bool("graphics", "vsync", Settings.enableVSync);
        mIniFile.set_string("graphics", "renderer", Settings.rendererName);
        mIniFile.set_int("graphics", "window_mode", static_cast<int>(Settings.windowMode));
        mIniFile.set_float("audio", "master_volume", Settings.masterVolume);
        mIniFile.set_float("audio", "music_volume", Settings.musicVolume);
        mIniFile.set_float("audio", "sfx_volume", Settings.sfxVolume);
        mIniFile.set_string("audio", "audio_device", Settings.audioDeviceName);

        std::string config_path = std::format("{}/{}", Platforms::GetConfigPath(mGamePackageID.c_str()), "settings.cfg");

        if (!CE::Ini::save_file(config_path, mIniFile)) {
            CE_LOG(LogLevel::Error, "[Settings Manager {}] Failed to save file!", mInstanceID);
        }
    }

    std::string SettingsManager::GetSettingPath() {
        std::string config_path = std::format("{}/{}", Platforms::GetConfigPath(mGamePackageID.c_str()), "settings.cfg");
        return config_path;
    }

    int SettingsManager::Custom_GetInteger(std::string key, std::string section, int fallback) {
        return mIniFile.get_int(section, key, fallback);
    }

    float SettingsManager::Custom_GetFloat(std::string key, std::string section, float fallback) {
        return mIniFile.get_float(section, key, fallback);
    }

    bool SettingsManager::Custom_GetBool(std::string key, std::string section, bool fallback) {
        return mIniFile.get_bool(section, key, fallback);
    }

    std::string SettingsManager::Custom_GetString(std::string key, std::string section, std::string fallback) {
        return mIniFile.get_string(section, key, fallback);
    }

    void SettingsManager::Custom_SetInteger(std::string key, std::string section, int value) {
        mIniFile.set_int(section, key, value);
    }

    void SettingsManager::Custom_SetFloat(std::string key, std::string section, float value) {
        mIniFile.set_float(section, key, value);
    }

    void SettingsManager::Custom_SetBool(std::string key, std::string section, bool value) {
        mIniFile.set_bool(section, key, value);
    }

    void SettingsManager::Custom_SetString(std::string key, std::string section, std::string value) {
        mIniFile.set_string(section, key, value);
    }
} // namespace CE::Settings
