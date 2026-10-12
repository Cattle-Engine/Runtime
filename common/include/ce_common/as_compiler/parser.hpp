#pragma once

#include <vector>

#include "ce_common/as_compiler/ast.hpp"
#include "ce_common/as_compiler/lexer.hpp"

namespace CE::Scripting::Impl::Parser {
    /**
     * @brief Parses the output of the lexer and builds an ASTModule
     * @return Returns an AST::ASTModule or throws CE::Runtime::Impl::Execptions::ParserError on errors
     */
    AST::ASTModule ParseLexerOutput(std::vector<Lexer::Token> tokens);
} // namespace CE::Scripting::Impl::Parser