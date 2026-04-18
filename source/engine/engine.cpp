#include "engine/engine.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/version.hpp"
#include "engine/common/tracelog.hpp"

namespace CE {
    Engine::Engine(int argv, char* argc[], std::string& datafilename) {
        CE::Log(CE::LogLevel::Info, "Cattle Engine");
        CE::Log(CE::LogLevel::Info, "CE Version: {}", CE::Version::engineVersionString);
    
    mDataFileName = datafilename;

    // Parse arguments and activate certain settings and shish
    // Read Gamedata.txt to what renderer to use here.
    // Create Renderer::GPUHandle here
    // idk what else to put here
    }

    bool Engine::CreateIntsance(std::string& name, 
            std::optional<std::string> datafilename = std::nullopt, bool debug)  {
        std::string file2use;

        if (datafilename.has_value()) {
            file2use = *datafilename;
        } else {
            file2use = mDataFileName;
        }

        try {
            InstanceHandle handle = std::make_unique<Instance>(file2use.c_str(), debug, mGPUHandle);
            mInstances[name] = std::move(handle);
        } catch (std::runtime_error& e) {
            CE::Log(CE::LogLevel::Fatal, "[Engine] Fatal error creating instance\n {}", e.what());
            return false;
        }

        CE::Log(LogLevel::Info, "[Engine] Created instance with ID of: {}", GLOBALINSTANCESCOUNTER);
        return true;
    }

    bool Engine::DestroyInstance(std::string& name) {
        auto handle = mInstances.find(name);

        if (handle != mInstances.end()) {
            CE::Log(LogLevel::Info, "[Engine] Deleting engine with ID of: {} ", 
            handle->second->GetInstanceID());
            handle->second.reset();
            mInstances.erase(name);
            return true;
        } else {
            CE::Log(LogLevel::Error, "[Engine] Instance {} does not exist!", name);
            return false;
        }
    }

    int Engine::UpdateInstance(std::string& name) {
        auto handle = mInstances.find(name);

        if (handle != mInstances.end()) {
            handle->second->Update();
        } else {
            CE::Log(LogLevel::Error, "[Engine] Instance {} does not exist!", name);
            return 0;
        }
    }

    int Engine::Run() {
        CE::Log(LogLevel::Info, "[Engine] Starting main loop");

        while (!mInstances.empty()) {
            for (auto it = mInstances.begin(); it != mInstances.end(); ) {
                if (it->second) {
                    it->second->Update();

                    if (it->second->ShouldExit()) {
                        CE::Log(LogLevel::Info,
                            "[Engine] Instance {} requested shutdown",
                            it->second->GetInstanceID());

                        it = mInstances.erase(it);
                    } else {
                        ++it;
                    }
                } else {
                    // Defensive cleanup
                    it = mInstances.erase(it);
                }
            }
        }

        CE::Log(LogLevel::Info, "[Engine] All instances closed. Shutting down.");
        return 0;
    }
}
