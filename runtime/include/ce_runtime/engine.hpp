#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "ce_runtime/common/misc/arguments.hpp"
#include "ce_runtime/common/misc/gameinfo.hpp"
#include "ce_runtime/instance/game_instance.hpp"
#include "ce_runtime/rendering/renderer.hpp"

namespace CE {
    class Engine {
      public:
        Engine(std::string datafilename, bool debug, const EngineArguements& args);
        ~Engine();
        bool CreateInstance(std::string name, bool debug, std::optional<std::string> datafilename = std::nullopt);
        bool DestroyInstance(std::string name);
        int UpdateInstance(std::string name);
        int Run();

      private:
        EngineArguements mEngineArgs;
        Renderer::GPUDeviceHandle mGPUHandle;
        RendererBackend mBackend;
        bool mRunning = true;
        std::string mDataFileName;
        std::unordered_map<std::string, InstanceHandle> mInstances;
        GameInfo mGameInfo;
    };
} // namespace CE