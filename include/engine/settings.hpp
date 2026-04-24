#pragma once

#include <string>
#include <cstdint>

#include "engine/common/ini.hpp"
#include "engine/common/gameinfo.hpp"

namespace CE {
    class Instance;
}

namespace CE::Settings {
    struct SettingsInfo {
        int windowHeight;
        int windowWidth;
        int maxFPS;
        bool enableVSync;
        std::string rendererName;
        bool fullscreen;
    };

    class SettingsManager {
        public:
            SettingsManager(const GameInfo& gameinfo, uint64_t instance_id);
            void SetInstance(Instance& instance);
            SettingsInfo Settings;
            
            bool ReloadSettings();
            int Custom_GetInteger(std::string key, std::string section, int fallback);
            float Custom_GetFloat(std::string key, std::string section, float fallback);
            bool Custom_GetBool(std::string key, std::string section, bool fallback);
            std::string Custom_GetString(std::string key, std::string section, std::string fallback);

            void Custom_SetInteger(std::string key, std::string section, int value);
            void Custom_SetFloat(std::string key, std::string section, float value);
            void Custom_SetBool(std::string key, std::string section, bool value);
            void Custom_SetString(std::string key, std::string section, std::string value);
            
            std::string GetSettingPath();

            void FlushSettings();

        private:
            bool Internal_ReloadSettings();
            CE::Instance* mInstance = nullptr;
            CE::Ini::IniFile mIniFile;
            CE::Ini::ParseError mParseError;
            CE::Ini::Options mOptions;
            std::string mPrevRendererName;
            std::string mGameName;
            GameInfo mGameInfo;
            uint64_t mInstanceID = 0;
    };
} 
