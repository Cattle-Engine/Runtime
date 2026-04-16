#pragma once

#include <string>
#include <unordered_map>

#include "engine/renderer.hpp"
#include "engine/instance.hpp"

namespace CE {
    class Engine {
            Engine(int argv, char* argc[], std::string& datafilename);
        private:
            Renderer::GPUDeviceHandle gGPUHandle;
            std::string gDataFileName;
            std::unordered_map<InstanceHandle, std::string> gInstances;
    };
}