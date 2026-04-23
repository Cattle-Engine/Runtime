#pragma once

#include <string>
#include <optional>
#include <unordered_map>

#include "engine/renderer.hpp"
#include "engine/instance.hpp"
#include "engine/common/gameinfo.hpp"

namespace CE {
    class Engine {
        public:
            Engine(int argc, char *argv[], std::string datafilename, bool debug);
            ~Engine();
            bool CreateInstance(std::string name, 
                bool debug, std::optional<std::string> datafilename = std::nullopt);
            bool DestroyInstance(std::string name);
            int UpdateInstance(std::string name);
            int Run();    
        private:
            Renderer::GPUDeviceHandle mGPUHandle;
            RendererBackend mBackend;
            bool mRunning = true;
            std::string mDataFileName;
            std::unordered_map<std::string, InstanceHandle> mInstances;
            GameInfo mGameInfo;
    };
}