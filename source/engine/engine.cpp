#include "engine/engine.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/version.hpp"

namespace CE {
    Engine::Engine(int argv, char* argc[], std::string& datafilename) {
        CE::Log(CE::LogLevel::Info, "Cattle Engine");
        CE::Log(CE::LogLevel::Info, "CE Version: {}", CE::Version::engineVersionString);
    }
}