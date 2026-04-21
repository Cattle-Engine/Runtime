#include "engine/engine.hpp"
#include <string_view>

int main(int argc, char *argv[]) {
    CE::Engine engine(argc, argv, "data.tcf", true);
    if (!engine.CreateInstance("main", true, "data.tcf")) {
        return 1;
    }
    if (argc >= 2 && std::string_view(argv[1]) == "--two") {
        if (!engine.CreateInstance("second", true, "data.tcf")) {
            return 1;
        }
    }
    return engine.Run();
}
