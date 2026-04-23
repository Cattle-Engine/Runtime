#include <format>

#include "engine/common/ini.hpp"
#include "engine/settings.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/platforms.hpp"

namespace CE::Settings {
    SettingsManager::SettingsManager(GameInfo gameinfo, uint64_t instance_id) {
        mGameName = gameinfo.gameNameString;
        mGameInfo = gameinfo;
        mInstanceID = instance_id;
        Internal_ReloadSettings();
    }

    bool SettingsManager::ReloadSettings() {
        CE::Log(LogLevel::Debug, "[Settings Manager {}] To change renderer you need to reload engine!");
        return Internal_ReloadSettings();
    }

    bool SettingsManager::Internal_ReloadSettings() {
        std::string config_path = std::format("{}/{}", Platforms::GetConfigPath(mGameName.c_str()), "settings.cfg");
        if(!CE::Ini::load_file(config_path, mIniFile, &mParseError)) {
            CE::Log(LogLevel::Error, "[Settings Manager {}] Setting parser error\n Line: {}, Column: {}, Message: {}", mInstanceID, mParseError.line,
                                        mParseError.column, mParseError.message);
            Settings.windowHeight = mGameInfo.windowHeight;
            Settings.windowWidth = mGameInfo.windowWidth;
            Settings.maxFPS = mGameInfo.maxFPS;
            Settings.enableVSync = mGameInfo.enableVSync;
            Settings.rendererName = mGameInfo.rendererName;
            Settings.fullscreen = mGameInfo.fullscreen;
            return false;
        }

        Settings.windowHeight = mIniFile.get_int("graphics", "window_height", mGameInfo.windowHeight);
        Settings.windowWidth = mIniFile.get_int("graphics", "window_width", mGameInfo.windowWidth);
        Settings.maxFPS = mIniFile.get_int("graphics", "max_fps", mGameInfo.maxFPS);
        Settings.enableVSync = mIniFile.get_bool("graphics", "vsync", mGameInfo.enableVSync);
        Settings.rendererName = mIniFile.get_string("graphics", "renderer", mGameInfo.rendererName);
        Settings.fullscreen = mIniFile.get_bool("graphics", "fullscreen", mGameInfo.fullscreen);
    }

    void SettingsManager::FlushSettings() {
        mIniFile.set_int("graphics", "window_height", Settings.windowHeight);
        mIniFile.set_int("graphics", "window_width", Settings.windowWidth);
        mIniFile.set_int("graphics", "max_fps", Settings.maxFPS);
        mIniFile.set_bool("graphics", "vsync", Settings.enableVSync);
        mIniFile.set_string("graphics", "renderer", Settings.rendererName);
        mIniFile.set_bool("graphics", "fullscreen", Settings.fullscreen);

        std::string config_path =
            std::format("{}/{}", Platforms::GetConfigPath(mGameName.c_str()), "settings.cfg");

        CE::Ini::save_file(config_path, mIniFile);
    }

    int SettingsManager::CustomS_GetInteger(std::string key, std::string section, int fallback) {
        return mIniFile.get_int(section, key, fallback);
    }

    float SettingsManager::CustomS_GetFloat(std::string key, std::string section, float fallback) {
        return mIniFile.get_float(section, key, fallback);
    }

    bool SettingsManager::CustomS_GetBool(std::string key, std::string section, bool fallback) {
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
}