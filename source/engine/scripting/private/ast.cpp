#include "engine/common/utils/hasher.hpp"
#include "engine/scripting/private/ast.hpp"

namespace CE::Scripting::Impl::AST {
    // forward declared to stop a circular dep
    void HashDeclaration(Utils::StreamingHasher& hasher, const ASTDeclaration& decl);
    void HashNamespace(Utils::StreamingHasher& hasher, const ASTNamespace& ns);
    
    uint64_t HashModule(const ASTModule& module) {
        Utils::StreamingHasher hasher;
        
        // hash imports
        for (const auto& imp : module.Imports) {
            hasher.AddString(imp.Module);
            if (imp.Symbol) hasher.AddString(*imp.Symbol);
        }
        
        // hash all declarations
        for (const auto& decl : module.Declarations) {
            HashDeclaration(hasher, decl);
        }
        
        return hasher.Finalize();
    }

    // helper function to easily hash stuff
    void HashTypeRef(Utils::StreamingHasher& hasher, const ASTTypeRef& type) {
        hasher.AddString(type.Name);
        hasher.AddValue(type.IsConst);
        hasher.AddValue(type.IsReference);
        hasher.AddValue(type.IsHandle);
        hasher.AddValue(type.ArrayDepth);
        hasher.AddValue(type.IsAuto);
    }
    
    void HashParameter(Utils::StreamingHasher& hasher, const ASTParameter& param) {
        HashTypeRef(hasher, param.Type);
        hasher.AddString(param.Name);
    }
    
    void HashFunction(Utils::StreamingHasher& hasher, const ASTFunction& func) {
        hasher.AddString(func.Name);
        HashTypeRef(hasher, func.ReturnType);
        
        // hash parameters
        for (const auto& param : func.Parameters) {
            HashParameter(hasher, param);
        }
        // hash the function body
        hasher.AddString(func.Body);
    }
    
    void HashGlobal(Utils::StreamingHasher& hasher, const ASTGlobal& global) {
        HashTypeRef(hasher, global.Type);
        hasher.AddString(global.Name);
        hasher.AddString(global.Initializer);
    }
    
    void HashType(Utils::StreamingHasher& hasher, const ASTType& type) {
        hasher.AddString(type.Name);
        hasher.AddString(type.Body);
    }

    void HashDeclaration(Utils::StreamingHasher& hasher, const ASTDeclaration& decl) {
        // hash common fields
        hasher.AddValue(static_cast<int>(decl.Type));
        hasher.AddValue(decl.Exported);
        hasher.AddString(decl.Name);
        hasher.AddString(decl.NameSpace);
        
        // hash the variant data based on type
        std::visit([&](const auto& data) {
            using T = std::decay_t<decltype(data)>;
            
            if constexpr (std::is_same_v<T, ASTFunction>) {
                HashFunction(hasher, data);
            } else if constexpr (std::is_same_v<T, ASTGlobal>) {
                HashGlobal(hasher, data);
            } else if constexpr (std::is_same_v<T, ASTType>) {
                HashType(hasher, data);
            } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTNamespace>>) {
                if (data) {
                    HashNamespace(hasher, *data);
                } else {
                    hasher.AddValue(0); // hash a null marker
                }
            }
        }, decl.Data);
    }
    
    void HashNamespace(Utils::StreamingHasher& hasher, const ASTNamespace& ns) {
        hasher.AddString(ns.Name);
        
        // hash all declarations in the namespace
        for (const auto& decl : ns.Declarations) {
            HashDeclaration(hasher, decl);
        }
    }
    
}