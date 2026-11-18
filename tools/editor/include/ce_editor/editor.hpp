#pragma once

#include "ce_runtime/instance/base_instance.hpp"

namespace CE::Editor {
    class EditorInstance : public ::CE::BaseInstance {
        public:
            EditorInstance(
                const char* data_file_path,
                bool debugmode,
                Renderer::GPUDeviceHandle& gpudevice,
                EngineArguements args
            ) {
                this->InitialiseBase(
                    debugmode,
                    gpudevice,
                    args,
                    std::string("")
                );
            }
    };
}