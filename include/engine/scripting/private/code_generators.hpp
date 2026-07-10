#pragma once

#include <string>

namespace CE::Scripting::Impl::CodeGenerators {
    // Although comments are stripped in the CE lexer, this is here for debugging purposes
    std::string GenerateComment(std::string comment);
}
