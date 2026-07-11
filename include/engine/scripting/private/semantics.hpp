#pragma once

#include <string>

#include "engine/common/fs/vfs.hpp"
#include "engine/scripting/private/ast.hpp"

namespace CE::Scripting::Impl::Semantics {
    class SymanticAnalyser {
        public:
            /**
             * @brief Creates a semantic analyser.
             *
             * @param vfs The virtual file system.
             */
            SymanticAnalyser(VFS::VFS& vfs);
                
            /**
             * @brief Analyses a module for semantic errors and adds the mono script if no errors
             *
             * @param module The module to analyse and add
             *
             * @return Does not return anything but throws CE::Scripting::Impl::Exceptions::SemanticError if an error happened 
            */
            void AddModule(AST::ASTModule& module, const std::string module_path);
            
            // Get the mono script for compilation to bytecode
            std::string GetMonoscript() const;
        private:
            struct ExportInfo {
              std::string OriginalName = "";
              std::string GeneratedName = "";
              AST::ASTDeclaration::Kind Type = AST::ASTDeclaration::Kind::Type;
              std::string Namespace = "";
              std::string Modulepath;
            };
            VFS::VFS& mVFS;

            std::string GenerateInternalName(const AST::ASTDeclaration& decl, const std::string& namespace_path, const std::string& module_hash);
            std::string GenerateSignatureHash(const AST::ASTFunction func);
    };
}
