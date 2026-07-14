#pragma once

#include <string>
#include <optional>
#include <unordered_map>

#include "engine/rendering/renderer.hpp"
#include "engine/common/misc/arguments.hpp"
#include "engine/instance.hpp"
#include "engine/common/misc/gameinfo.hpp"

namespace CE {
    class Engine {
        public:
            Engine(std::string datafilename, bool debug, const EngineArguements& args);
            ~Engine();
            bool CreateInstance(std::string name, 
                bool debug, std::optional<std::string> datafilename = std::nullopt);
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
}