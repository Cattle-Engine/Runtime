#pragma once

#include <memory>

#include <SDL3/SDL.h>

#include "engine/instance/base_instance.hpp"
#include "engine/scripting/angelscript.hpp"
#include "engine/settings.hpp"
#include "engine/ui/debug_window.hpp"
#include "engine/ui/angelscript_debugger_window.hpp"

namespace CE {
    class GameInstance final : public BaseInstance {
      public:
        GameInstance(
          const char* data_file_path,
          bool debugmode,
          Renderer::GPUDeviceHandle& gpudevice,
          EngineArguements args
        );

        ~GameInstance() override;

        GameInfo& GetGameInfo() override;
        Settings::SettingsManager& GetSettingsManager() override;

      protected:
        InstanceRuntimeConf PrepareRuntimeData() override;

        void OnInitialise() override;
        void HandleWindowEvents() override;
        void ApplySettingsReload() override;
        bool OnLogic() override;
        bool OnDraw2D() override;
        bool OnImGui() override;

      private:
        InstanceRuntimeConf BuildRuntimeConf();

        std::unique_ptr<GameInfo> mGameInfo;
        std::unique_ptr<Settings::SettingsManager> mSettingsManager;
        std::unique_ptr<Scripting::Runtime> mScriptingManager;
        std::unique_ptr<UI::DebugWindow> mDebugWindow;
        std::unique_ptr<UI::AngelscriptDebuggerWindow> mASDebuggerWindow;
    };

    using InstanceHandle = std::unique_ptr<GameInstance>;
} // namespace CE