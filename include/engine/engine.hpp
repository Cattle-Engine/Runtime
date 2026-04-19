#pragma once

#include <string>
#include <optional>
#include <unordered_map>

#include "engine/renderer.hpp"
#include "engine/instance.hpp"
#include "engine/gameinfo.hpp"

namespace CE {
    class Engine {
            Engine(int argv, char* argc[], std::string& datafilename, bool debug);
            bool CreateIntsance(std::string& name, 
                std::optional<std::string> datafilename = std::nullopt, bool debug);
            bool DestroyInstance(std::string& name);
            int UpdateInstance(std::string& name);
            int Run();            
        private:
            Renderer::GPUDeviceHandle mGPUHandle;
            RendererBackend mBackend;
            std::string mDataFileName;
            std::unordered_map<std::string ,InstanceHandle> mInstances;
            GameInfo mGameInfo;
    };
}