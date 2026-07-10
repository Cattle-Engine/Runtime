#include <memory>

#include "engine/common/utils/hasher.hpp"
#include "engine/scripting/private/ast.hpp"
#include "engine/scripting/private/semantics.hpp"

namespace {
    constexpr char kInternalFunctionPrefix[] = "__ce_mod_f_";
    constexpr char kInternalGlobalPrefix[] = "__ce_mod_g_";
    constexpr char kInternalTypePrefix[] = "__ce_mod_t_";
}

namespace CE::Scripting::Impl::Semantics {
    SymanticAnalyser::SymanticAnalyser(bool generate_debug_comments, VFS::VFS& vfs) :
        mGenerateDebugComments(generate_debug_comments), mVFS(vfs) {}
        
        std::string SymanticAnalyser::GenerateInternalName(const AST::ASTDeclaration& decl, const std::string& namespace_path)  {
            std::string prefix;
            std::string suffix;
            
            std::string namespace_hash = Utils::Hash2String(Utils::Hash64(namespace_path));
            std::string symbol_hash = Utils::Hash2String(Utils::Hash64(decl.Name));
            
            switch (decl.Kind) {
                case AST::ASTDeclaration::Kind::Function: {
                     auto func = std::get<AST::ASTFunction>(decl.Data);
                     
                     std::string signature_hash = "" // TODO Add a GenerateSignatureHash
                     std::string return_type  = func.ReturnType.Name;
                     
                     prefix = kInternalFunctionPrefix;
                }
            }
        }
}