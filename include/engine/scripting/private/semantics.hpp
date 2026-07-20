#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/scripting/private/ast.hpp"

namespace CE::Scripting::Impl::Semantics {
    struct FunctionSignature {
        AST::ASTTypeRef ReturnType;
        std::vector<AST::ASTTypeRef> Parameters;
    };

    struct Symbol {
        AST::ASTDeclaration::Kind Kind;
        std::string QualifiedName; // eg: "Foo::Test"
        std::string InternalName;  // the mangled symbol
        std::string SourceModule;  // source module
        SourceLocation Location;
        bool Exported = false;
        FunctionSignature Signature; // only used for functions
    };

    class SymbolTable {
      public:
        // returns false if a colliding symbol already exists
        bool Declare(Symbol symbol);

        const Symbol *Find(const std::string &qualified_name) const;
        std::vector<const Symbol *> FindOverloads(const std::string &qualified_name) const;

      private:
        std::unordered_map<std::string, std::vector<Symbol>> mSymbols;
    };

    class SymanticAnalyser {
      public:
        /**
         * @brief Creates a semantic analyser.
         *
         * @param vfs The virtual file system.
         */
        SymanticAnalyser(VFS::VFS &vfs);

        /**
         * @brief Analyses a module for semantic errors and adds to the mono script if no errors
         *
         * @param module The module to analyse and add
         *
         * @return Does not return anything but throws CE::Scripting::Impl::Exceptions::SemanticError if an error
         * happened
         */
        void CheckModule(AST::ASTModule &module, const std::string module_path);

        const Symbol *FindSymbol(const std::string &qualified_name, const std::string &module_path = "") const;
        const Symbol *ResolveSymbol(const std::string &module_path, const std::string &qualified_name) const;

        const Symbol *ResolveFunction(const std::string &module_path, const std::string &name,
                                      const std::vector<AST::ASTTypeRef> &arguments) const;

        std::vector<const Symbol *> ResolveOverloads(const std::string &module_path,
                                                     const std::string &qualified_name) const;

        AST::ASTTypeRef GetMemberType(const std::string &class_name, const std::string &member_name,
                                      const std::string &module_path) const;
        AST::ASTTypeRef GetGlobalType(const Symbol *symbol) const;

        const std::vector<std::string> &GetEmissionOrder() const {
            return mEmissionOrder;
        }
        const std::unordered_map<std::string, AST::ASTModule> &GetParsedModules() const {
            return mParsedModules;
        }

        const Symbol *FindDeclarationSymbol(const std::string &qualified_name, const std::string &module_path,
                                            const AST::ASTDeclaration &decl) const;

      private:
        struct ExportInfo {
            std::string OriginalName;
            std::string InternalName;
            AST::ASTDeclaration::Kind Type;
            std::string Namespace;
            std::string Modulepath;
            FunctionSignature Signature;
        };

        VFS::VFS &mVFS;
        // Each module owns its private declarations. Cross-module visibility is
        // controlled exclusively by mModuleExports.
        std::unordered_map<std::string, SymbolTable> mModuleSymbols;
        std::unordered_map<std::string, std::vector<ExportInfo>> mModuleExports;
        std::unordered_map<std::string, bool> mAnalyzedModules; // false = in-progres, true = done
        std::unordered_map<std::string, AST::ASTModule> mParsedModules;
        std::vector<std::string> mEmissionOrder;
        std::unordered_map<std::string, std::vector<ExportInfo>> mModuleUsing;

        void VisitDeclaration(AST::ASTDeclaration &decl, const std::string &enclosing_namespace,
                              const std::string &module_hash, const std::string &module_path);
        AST::ASTModule LoadAndParseModule(const std::string &module_path);

        std::string GenerateInternalName(const AST::ASTDeclaration &decl, const std::string &namespace_path,
                                         const std::string &module_hash);
        std::string GenerateSignatureHash(const AST::ASTFunction func) const;
    };
} // namespace CE::Scripting::Impl::Semantics
