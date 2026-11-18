#pragma once

#include <SDL3/SDL.h>

#include "ce_runtime/common/fs/vfs.hpp"
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
                    std::string(std::string(SDL_GetBasePath()) + "/editor_data")
                );
            }
        protected:
            InstanceRuntimeConf PrepareRuntimeData()  override;
            int MountData(const std::string& path) override;
        private:
            Common::FS::VFS::MountPoint::MountID mRootMountPointID{0};
    };
}