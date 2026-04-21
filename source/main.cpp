#include "engine/engine.hpp"
#include "engine/common/tracelog.hpp"

int main(int argc, char *argv[]) {
    CE::Engine engine(argc, argv, "data.tcf", true);
    if (!engine.CreateInstance("main", true, "data.tcf")) {
        return 1;
    }
    return engine.Run();
}