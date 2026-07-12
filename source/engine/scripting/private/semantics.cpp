#include "engine/common/utils/hasher.hpp"
#include "engine/scripting/private/ast.hpp"
#include "engine/scripting/private/semantics.hpp"

namespace {
    constexpr char kInternalFunctionPrefix[] = "__ce_mod_f_";
    constexpr char kInternalGlobalPrefix[] = "__ce_mod_g_";
    constexpr char kInternalTypePrefix[] = "__ce_mod_t_";
}

namespace CE::Scripting::Impl::Semantics {
    bool SymbolTable::Declare(Symbol symbol) {
        auto& bucket = mSymbols[symbol.QualifiedName];

        if (symbol.Kind != AST::ASTDeclaration::Kind::Function) {
            if (!bucket.empty()) {
                return false; // globals/types/namespaces can't collide at all
            }
            bucket.push_back(std::move(symbol));
            return true;
        }

        // functions, collide only if InternalName matches (same signature)
        for (const auto& existing : bucket) {
            if (existing.InternalName == symbol.InternalName) {
                return false;
            }
        }
        bucket.push_back(std::move(symbol));
        return true;
    }

    SymanticAnalyser::SymanticAnalyser(VFS::VFS& vfs) :
        mVFS(vfs) {}
        
        void SymanticAnalyser::CheckModule(AST::ASTModule& module, const std::string module_path) {
            std::string module_hash = Utils::Hash2String(AST::HashModule(module));
        }
        
        std::string SymanticAnalyser::GenerateSignatureHash(const AST::ASTFunction func) {
            std::string signature;
            
            signature += func.ReturnType.Name;
            
            // add parameter types
            for (const auto& param : func.Parameters) {
                signature += "_" + param.Type.Name
                        + (param.Type.IsConst ? "C" : "")
                        + (param.Type.IsHandle ? "H" : "")
                        + (param.Type.IsReference ? "R" : "")
                        + std::to_string(param.Type.ArrayDepth);
            }
            
            return Utils::Hash2String(Utils::Hash64(signature));
        }
        
        std::string SymanticAnalyser::GenerateInternalName(const AST::ASTDeclaration& decl, const std::string& 
        namespace_path, const std::string& module_hash)  {
            std::string prefix;
            std::string suffix;
            
            std::string namespace_hash = Utils::Hash2String(Utils::Hash64(namespace_path));
            std::string symbol_hash = Utils::Hash2String(Utils::Hash64(decl.Name));
            
            switch (decl.Type) {
                case AST::ASTDeclaration::Kind::Function: {
                     auto func = std::get<AST::ASTFunction>(decl.Data);
                     
                     std::string signature_hash = GenerateSignatureHash(func);
                     std::string return_type  = func.ReturnType.Name;
                     
                     prefix = kInternalFunctionPrefix;
                     suffix = module_hash + "_" +namespace_hash + "_" + symbol_hash + "_" + signature_hash + "_" 
                     + return_type;
                     break;
                }
                
                case AST::ASTDeclaration::Kind::Global: {
                    auto global = std::get<AST::ASTGlobal>(decl.Data);
                    std::string type_hash = Utils::Hash2String(Utils::Hash64(global.Type.Name));
                    
                    prefix = kInternalGlobalPrefix;
                    suffix = module_hash + "_" + namespace_hash + "_" + symbol_hash + "_" + type_hash;
                    break;
                }
                
                case AST::ASTDeclaration::Kind::Type: {
                    prefix = kInternalTypePrefix;
                    suffix = module_hash + "_" + namespace_hash + "_" + symbol_hash;
                    break;
                }
                
                 case AST::ASTDeclaration::Kind::Namespace:
                     // namespaces don't generate symbools directly
                     return decl.Name;
                
                 default:
                     return decl.Name;
            }
            
            return prefix + suffix;
        }
}