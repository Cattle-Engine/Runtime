#include "engine/scripting//private/code_generators.hpp"

namespace CE::Scripting::Impl::CodeGenerators {
    std::string GenerateComment(std::string comment) {
        return "// " + comment;
    }


}
