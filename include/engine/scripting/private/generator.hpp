#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "engine/scripting/private/ast.hpp"
#include "engine/scripting/private/semantics.hpp"

namespace CE::Scripting::Impl::Codegen {
    class Generator {
      public:
        explicit Generator(const Semantics::SymanticAnalyser &analyser) : mAnalyser(analyser) {}

        std::string GenerateMonoScript(const std::vector<std::string> &emission_order,
                                       const std::unordered_map<std::string, AST::ASTModule> &parsed_modules) const;

      private:
        const Semantics::SymanticAnalyser &mAnalyser;

        std::string EmitDeclaration(const AST::ASTDeclaration &declaration, const std::string &module_path,
                                    const std::string &name_space) const;
        std::vector<Lexer::Token> RewriteBody(const std::vector<Lexer::Token> &tokens, const AST::ASTFunction *function,
                                              const std::string &module_path, const std::string &name_space) const;
        std::string EmitTypeRef(const AST::ASTTypeRef &type, const std::string &module_path,
                                const std::string &name_space) const;
        static std::string JoinTokens(const std::vector<Lexer::Token> &tokens);
    };
} // namespace CE::Scripting::Impl::Codegen
