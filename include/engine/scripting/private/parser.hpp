#pragma once 

#include <vector>

#include "engine/scripting/private/ast.hpp"
#include "engine/scripting/private/lexer.hpp"

namespace CE::Scripting::Impl::Parser {
    /**
     * @brief Parses the output of the lexer and builds an ASTModule
     * @return Returns an AST::ASTModule or throws CE::Runtime::Impl::Execptions::ParserError on errors
     */
    AST::ASTModule ParseLexerOutput(std::vector<Lexer::Token> tokens);
}