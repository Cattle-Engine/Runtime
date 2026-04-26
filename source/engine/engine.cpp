#include <format>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "engine/engine.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/version.hpp"
#include "engine/renderer.hpp"
#include "engine/common/renderer_name_2_string.hpp"
#include "engine/common/events.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/settings.hpp"
#include "engine/bootstrap/engine.hpp"

namespace CE {
   Engine::Engine(int argc, char *argv[],
               std::string datafilename,
               bool debug) {
        CE::Log(CE::LogLevel::Info, "Cattle Engine");
        CE::Log(CE::LogLevel::Info, "CE Version: {}", CE::Version::engineVersionString);

        // Parse arguments and activate certain settings and shish

        const char* base = SDL_GetBasePath();
        if (!base) {
            CE::Log(LogLevel::Fatal, "[Engine] SDL_GetBasePath() returned NULL");
            base = "";
        }

        mDataFileName =  std::format("{}{}", base, datafilename);
        CE::Log(CE::LogLevel::Info, "[Engine] Game-data filepath: {}", mDataFileName);

        Bootstrap::Engine::GetGameInfo(mGameInfo, mDataFileName, debug);
        
        CE::Settings::SettingsManager msettings(mGameInfo, 1714284757 /*Why this large number? Its ENGINEC encoded to smth like base26*/);

        Common::RendererName2String(msettings.Settings.rendererName, mBackend);
        CE::Log(LogLevel::Info, "[Engine] Rendererbackend name: {}", msettings.Settings.rendererName);
        if (msettings.Settings.rendererName != "None") {
            SDL_Init(SDL_INIT_VIDEO);
        }
        CE::Log(LogLevel::Info, "[Engine] Creating GPU handle");
        mGPUHandle = Renderer::CreateGPUDevice(mBackend, true);
        
        if (mGPUHandle == nullptr) {
            CE::Log(LogLevel::Fatal, "[Engine] Got a nullptr GPU handle");
        }
        
        CE::Log(LogLevel::Info, "[Engine] Created GPU handle");
        // idk what else to put here
    }

    bool Engine::CreateInstance(std::string name, 
            bool debug, std::optional<std::string> datafilename)  {
        std::string file2use;
        CE::Log(LogLevel::Info, "[Engine] Attempting to make instance");
        if (datafilename) {
            CE::Log(LogLevel::Info, "[Engine] Data file wasn't nullptr");
            file2use = *datafilename;
        } else {
            file2use = mDataFileName;
            CE::Log(LogLevel::Info, "[Engine] Data file was nullptr");
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

    bool Engine::DestroyInstance(std::string name) {
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

    int Engine::UpdateInstance(std::string name) {
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

        while (mRunning && !mInstances.empty()) {
            Events::Update();

            for (const auto& e : CE::Events::gEvents) {
                if (e.type == SDL_EVENT_QUIT) {
                    CE::Log(LogLevel::Info, "[Engine] Quit event received");
                    mRunning = false;
                }
            }

            for (auto it = mInstances.begin(); it != mInstances.end(); ) {
                if (!it->second) {
                    it = mInstances.erase(it);
                    continue;
                }

                if (it->second->ShouldExit()) {
                    CE::Log(LogLevel::Info,
                        "[Engine] Instance {} requested shutdown",
                        it->second->GetInstanceID());

                    it = mInstances.erase(it);
                    continue;
                }

                it->second->Update();
                ++it;
            }
        }

        CE::Log(LogLevel::Info, "[Engine] All instances closed. Shutting down.");
        return 0;
    }

    Engine::~Engine() {
        for (auto& [name, instanceinfo] : mInstances) {
            instanceinfo.reset();         
        }
        mInstances.clear();
    
        Renderer::DestroyGPUDevice(mGPUHandle);
    }
}
