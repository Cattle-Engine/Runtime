#pragma once

#include <string>

#include "engine/common/ini.hpp"
#include "engine/common/gameinfo.hpp"

namespace CE::Settings {
    struct SettingsInfo {
        int windowHeight;
        int windowWidth;
        int maxFPS;
        bool enableVSync;
        std::string rendererName;
        bool fullscreen;
        bool resizableWindow;
    };

    class SettingsManager {
        public:
            SettingsManager(std::string game_name, GameInfo gameinfo);
            SettingsInfo Settings;
            
            bool ReloadSettings();
            int CustomS_GetInteger(std::string key, std::string section, int fallback);
            float CustomS_GetFloat(std::string key, std::string section, float fallback);
            bool CustomS_GetBool(std::string key, std::string section, bool fallback);
            std::string Custom_GetString(std::string key, std::string section, std::string fallback);

        private:
            CE::Ini::IniFile mIniFile;
            CE::Ini::ParseError mParseError;
            CE::Ini::Options mOptions;
            std::string mPrevRendererName;
            std::string mGameName;
            GameInfo mGameInfo;
    };
} 