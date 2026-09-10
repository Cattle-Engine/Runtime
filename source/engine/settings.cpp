#include "engine/settings.hpp"

#include <algorithm>
#include <format>

#include "engine/common/fs/ini.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/window.hpp"
#include "engine/instance.hpp"
#include "engine/platforms.hpp"

namespace CE::Settings {
    SettingsManager::SettingsManager(const GameInfo& gameinfo, uint64_t instance_id) {
        mGameName = gameinfo.gameNameString;
        mGameInfo = gameinfo;
        mInstanceID = instance_id;
        Internal_ReloadSettings();
    }

    void SettingsManager::SetInstance(CE::Instance& instance) {
        mInstance = &instance;
    }

    bool SettingsManager::ReloadSettings() {
        CE_LOG(LogLevel::Debug, "[Settings Manager {}] To change renderer you need to reload engine!", mInstanceID);
        bool reloaded = Internal_ReloadSettings();
        if (mInstance != nullptr) {
            mInstance->ReloadSettings();
        }
        return reloaded;
    }

    bool SettingsManager::Internal_ReloadSettings() {
        std::string config_path = std::format("{}/{}", Platforms::GetConfigPath(mGameName.c_str()), "settings.cfg");
        if (!CE::Ini::load_file(config_path, mIniFile, &mParseError)) {
            CE_LOG(LogLevel::Error, "[Settings Manager {}] Setting parser error\n Line: {}, Column: {}, Message: {}",
                   mInstanceID, mParseError.line, mParseError.column, mParseError.message);
            Settings.windowHeight = mGameInfo.windowHeight;
            Settings.windowWidth = mGameInfo.windowWidth;
            Settings.maxFPS = mGameInfo.maxFPS;
            Settings.enableVSync = mGameInfo.enableVSync;
            Settings.rendererName = mGameInfo.rendererName;
            Settings.windowMode = mGameInfo.windowMode;
            Settings.masterVolume = 1.0f;
            Settings.musicVolume = 1.0f;
            Settings.sfxVolume = 1.0f;
            return false;
        }
        int window_mode;
        Settings.windowHeight =
            static_cast<int>(std::clamp<int64_t>(mIniFile.get_int("graphics", "window_height", mGameInfo.windowHeight),
                                                 mGameInfo.minWindowHeight, mGameInfo.maxWindowHeight));

        Settings.windowWidth =
            static_cast<int>(std::clamp<int64_t>(mIniFile.get_int("graphics", "window_width", mGameInfo.windowWidth),
                                                 mGameInfo.minWindowWidth, mGameInfo.maxWindowWidth));
        Settings.maxFPS = mIniFile.get_int("graphics", "max_fps", mGameInfo.maxFPS);
        Settings.enableVSync = mIniFile.get_bool("graphics", "vsync", mGameInfo.enableVSync);
        Settings.rendererName = mIniFile.get_string("graphics", "renderer", mGameInfo.rendererName);
        window_mode = mIniFile.get_int("graphics", "window_mode", static_cast<int>(mGameInfo.windowMode));
        Settings.masterVolume = mIniFile.get_float("audio", "master_volume", 1.0f);
        Settings.musicVolume = mIniFile.get_float("audio", "music_volume", 1.0f);
        Settings.sfxVolume = mIniFile.get_float("audio", "sfx_volume", 1.0f);
        
        if (window_mode >= 0 && window_mode <= 2) {
            Settings.windowMode = static_cast<Common::Window::WindowMode>(window_mode);
        } else {
            CE_LOG(LogLevel::Error, "[Settings] Window_Mode is out of bounds, using game-info default)");
            CE_LOG(LogLevel::Debug, "0 = Fullscreen, 1 = Borderless, 2 = Windowed");
            Settings.windowMode = mGameInfo.windowMode; 
        }
        
        return true;
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

        std::string config_path = std::format("{}/{}", Platforms::GetConfigPath(mGameName.c_str()), "settings.cfg");

        if (!CE::Ini::save_file(config_path, mIniFile)) {
            CE_LOG(LogLevel::Error, "[Settings Manager {}] Failed to save file!", mInstanceID);
        }
    }

    std::string SettingsManager::GetSettingPath() {
        std::string config_path = std::format("{}/{}", Platforms::GetConfigPath(mGameName.c_str()), "settings.cfg");
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
