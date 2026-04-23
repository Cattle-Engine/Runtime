#include "engine/common/ini.hpp"
#include "engine/settings.hpp"
#include "engine/platforms.hpp"

namespace CE::Settings {
    SettingsManager::SettingsManager(std::string game_name, GameInfo gameinfo) {
        mGameName = game_name;
        mGameInfo = gameinfo;
    }

    bool SettingsManager::ReloadSettings() {
        Settings.enableVSync = mIniFile.get_bool("", "VSync", mGameInfo.enableVSync);
    }
}