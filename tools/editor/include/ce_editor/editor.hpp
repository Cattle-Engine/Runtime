#pragma once

#include "engine/instance.hpp"

namespace CE::Editor {
    class EditorInstance : Instance {
        public:
            EditorInstance(
                const char* data_file_path,
                bool debugmode,
                Renderer::GPUDeviceHandle& gpudevice,
                EngineArguements args
            ) : Instance(data_file_path, debugmode, gpudevice, args) {}
    };
}