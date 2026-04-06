#include "engine/bootstrap.hpp"
#include "engine/core.hpp"

namespace CE::Bootstrap {
    void Bootstrap() {
        CE::Bootstrap::GameSetup::SetupGameData(); // This is required for opening the vfs to get config
        CE::Bootstrap::GameSetup::SetupGameInfo(); // Get the game info, name, window stuff etc etc
        CE::Bootstrap::Video::Init(); // Create window and set the renderer
        CE::main();
    }
}