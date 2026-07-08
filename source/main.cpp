#include "engine/engine.hpp"
#include "engine/memory/allocator.hpp"
#include "engine/common/misc/error_box.hpp"

int main(int argc, char *argv[]) {
    try {
        CE::Memory::EnableTracking(true);
        CE::Engine engine(argc, argv, "data.tcf", true);
        if (!engine.CreateInstance("main", true)) return 1;
        return engine.Run();
    } catch (std::runtime_error& e) {
        ShowError(e.what());
        return 1;
    }
}
