#include "engine/engine.hpp"
#include <string_view>

int main(int argc, char *argv[]) {
    CE::Engine engine(argc, argv, "data.tcf", true);
    if (!engine.CreateInstance("main", true)) {
        return 1;
    }
    return engine.Run();
}
