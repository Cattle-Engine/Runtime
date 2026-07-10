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
             * @param generate_debug_comments Add comments to the monofile for debugging purposes.
             * @param vfs The virtual file system.
             */
            SymanticAnalyser(bool generate_debug_comments, VFS::VFS& vfs);
                
            /**
             * @brief Analyses a module for semantic errors and adds the mono script if no errors
             *
             * @param module The module to analyse and add
             *
             * @return Does not return anything but throws CE::Scripting::Impl::Exceptions::SemanticError if an error happened 
            */
            void AddModule(AST::ASTModule& module);
            
            // Get the mono script for compilation to bytecode
            std::string GetMonoscript() const;
        private:
            bool mGenerateDebugComments = false;
            VFS::VFS& mVFS;
            // Analysed scripts combined together into 1 giant script
            std::string mMonoScript;
            
            std::string GenerateInternalName(const AST::ASTDeclaration& decl, const std::string& namespace_path);
    };
}
