#include "engine/scripting/private/semantics.hpp"

#include <algorithm>
#include <unordered_set>

#include "engine/common/utils/hasher.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/scripting/private//parser.hpp"
#include "engine/scripting/private/ast.hpp"
#include "engine/scripting/private/common.hpp"
#include "engine/scripting/private/exceptions.hpp"
#include "engine/scripting/private/lexer.hpp"

namespace CE::Scripting::Impl::Semantics {
    namespace {
        constexpr char kInternalFunctionPrefix[] = "__ce_mod_f_";
        constexpr char kInternalGlobalPrefix[] = "__ce_mod_g_";
        constexpr char kInternalTypePrefix[] = "__ce_mod_t_";

        std::string StripNamespace(const std::string &name) {
            size_t pos = name.rfind("::");
            if (pos != std::string::npos) {
                return name.substr(pos + 2);
            }
            return name;
        }

        const AST::ASTDeclaration *FindDeclarationInDecls(const std::vector<AST::ASTDeclaration> &decls,
                                                          const std::string &target_qualified_name,
                                                          const std::string &current_namespace) {
            for (const auto &decl : decls) {
                std::string full_name = current_namespace.empty() ? decl.Name : current_namespace + "::" + decl.Name;
                if (decl.Type == AST::ASTDeclaration::Kind::Namespace) {
                    auto ns = std::get<std::shared_ptr<AST::ASTNamespace>>(decl.Data);
                    if (const auto *found =
                            FindDeclarationInDecls(ns->Declarations, target_qualified_name, full_name)) {
                        return found;
                    }
                } else {
                    if (full_name == target_qualified_name) {
                        return &decl;
                    }
                }
            }
            return nullptr;
        }

        const AST::ASTDeclaration *FindDeclarationInModule(const AST::ASTModule &module,
                                                           const std::string &qualified_name) {
            return FindDeclarationInDecls(module.Declarations, qualified_name, "");
        }
    } // namespace
    void AddFunctionOverloads(std::vector<const Symbol *> &result, const SymbolTable &table, const std::string &name) {
        auto overloads = table.FindOverloads(name);

        for (const auto *symbol : overloads) {
            if (symbol->Kind == AST::ASTDeclaration::Kind::Function) {
                result.push_back(symbol);
            }
        }
    }

    // Symbol table functions
    bool SymbolTable::Declare(Symbol symbol) {
        auto &bucket = mSymbols[symbol.QualifiedName];

        if (symbol.Kind != AST::ASTDeclaration::Kind::Function) {
            if (!bucket.empty()) {
                return false; // globals/types/namespaces can't collide at all
            }
            bucket.push_back(std::move(symbol));
            return true;
        }

        // functions, collide only if InternalName matches (same signature)
        for (const auto &existing : bucket) {
            if (existing.InternalName == symbol.InternalName) {
                return false; // identical signature already delcared
            }
        }
        bucket.push_back(std::move(symbol));
        return true;
    }

    const Symbol *SymbolTable::Find(const std::string &qualified_name) const {
        auto it = mSymbols.find(qualified_name);

        if (it == mSymbols.end() || it->second.empty())
            return nullptr;

        if (it->second.size() > 1)
            return nullptr; // ambiguous overload set

        return &it->second.front();
    }

    std::vector<const Symbol *> SymbolTable::FindOverloads(const std::string &qualified_name) const {
        std::vector<const Symbol *> result;
        auto it = mSymbols.find(qualified_name);
        if (it == mSymbols.end()) {
            return result;
        }
        for (const auto &sym : it->second) {
            result.push_back(&sym);
        }
        return result;
    }

    // Symantic parser functions
    SymanticAnalyser::SymanticAnalyser(VFS::VFS &vfs) : mVFS(vfs) {}

    std::string SymanticAnalyser::GenerateSignatureHash(const AST::ASTFunction func) const {
        std::string signature;

        for (const auto &param : func.Parameters) {
            signature += "_" + param.Type.Name + (param.Type.IsConst ? "C" : "") + (param.Type.IsHandle ? "H" : "") +
                         (param.Type.IsReference ? "R" : "") + std::to_string(param.Type.ArrayDepth) + param.Direction;
        }

        return Utils::Hash2String(Utils::Hash64(signature));
    }

    std::string SymanticAnalyser::GenerateInternalName(const AST::ASTDeclaration &decl,
                                                       const std::string &namespace_path,
                                                       const std::string &module_hash) {
        std::string prefix;
        std::string suffix;

        std::string namespace_hash = Utils::Hash2String(Utils::Hash64(namespace_path));
        std::string symbol_hash = Utils::Hash2String(Utils::Hash64(decl.Name));

        switch (decl.Type) {
        case AST::ASTDeclaration::Kind::Function: {
            auto func = std::get<AST::ASTFunction>(decl.Data);

            std::string signature_hash = GenerateSignatureHash(func);

            prefix = kInternalFunctionPrefix;
            suffix = module_hash + "_" + namespace_hash + "_" + symbol_hash + "_" + signature_hash + "_";
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

    void SymanticAnalyser::VisitDeclaration(AST::ASTDeclaration &decl, const std::string &enclosing_namespace,
                                            const std::string &module_hash, const std::string &module_path) {
        if (decl.Type == AST::ASTDeclaration::Kind::Namespace) {
            auto ns = std::get<std::shared_ptr<AST::ASTNamespace>>(decl.Data);
            std::string nested_ns = enclosing_namespace.empty() ? ns->Name : enclosing_namespace + "::" + ns->Name;

            for (auto &child : ns->Declarations) {
                VisitDeclaration(child, nested_ns, module_hash, module_path);
            }
            return;
        }

        const std::string &full_namespace = enclosing_namespace;

        std::string qualified_name = full_namespace.empty() ? decl.Name : full_namespace + "::" + decl.Name;
        std::string internal_name = GenerateInternalName(decl, full_namespace, module_hash);

        Symbol symbol{decl.Type, qualified_name, internal_name, module_path, decl.Location, decl.Exported};

        if (decl.Type == AST::ASTDeclaration::Kind::Function) {
            const auto &function = std::get<AST::ASTFunction>(decl.Data);

            symbol.Signature.ReturnType = function.ReturnType;

            for (const auto &parameter : function.Parameters) {
                symbol.Signature.Parameters.push_back(parameter.Type);
            }
        }

        if (!mModuleSymbols[module_path].Declare(symbol)) {
            throw Exceptions::SemanticError(
                "Redeclaration of '" + qualified_name + "'" +
                    (decl.Type == AST::ASTDeclaration::Kind::Function ? " with an identical parameter signature" : ""),
                decl.Location);
        }

        if (decl.Exported) {
            ExportInfo info{decl.Name, internal_name, decl.Type, full_namespace, module_path};

            if (decl.Type == AST::ASTDeclaration::Kind::Function) {
                info.Signature = symbol.Signature;
            }

            mModuleExports[module_path].push_back(std::move(info));
        }
    }

    void SymanticAnalyser::CheckModule(AST::ASTModule &module, const std::string module_path) {
        auto state = mAnalyzedModules.find(module_path);
        if (state != mAnalyzedModules.end()) {
            if (!state->second) {
                throw Exceptions::SemanticError("Circular import detected involving module '" + module_path + "'", {});
            }
            return; // already analysed
        }
        mAnalyzedModules[module_path] = false;

        std::string module_hash = Utils::Hash2String(AST::HashModule(module));

        for (auto &import : module.Imports) {
            std::string imported_path = Common::Import2Path(import, mVFS);

            // check if this is a file import (symbol present and path exists)
            if (import.Symbol.has_value() && !imported_path.empty() && mVFS.FileExists(imported_path.c_str())) {
                import.IsFileImport = true;
            }

            if (!mAnalyzedModules.count(imported_path)) {
                AST::ASTModule imported = LoadAndParseModule(imported_path);
                CheckModule(imported, imported_path);
            } else if (!mAnalyzedModules[imported_path]) {
                throw Exceptions::SemanticError(
                    "Circular import detected: '" + module_path + "' -> '" + imported_path + "'", import.Location);
            }

            const auto &imported_exports = mModuleExports[imported_path];

            if (import.IsUsing) {
                // using: bring symbols into scope )
                if (import.IsFileImport) {
                    // using tests::testing; (testing is a file  bring all exports from that file
                    for (const auto &exp : imported_exports) {
                        mModuleUsing[module_path].push_back(exp);
                    }
                } else if (import.Symbol.has_value()) {
                    // using module::symbol;
                    auto match = std::find_if(imported_exports.begin(), imported_exports.end(),
                                              [&](const ExportInfo &e) { return e.OriginalName == *import.Symbol; });
                    if (match == imported_exports.end()) {
                        throw Exceptions::SemanticError("Module '" + imported_path + "' has no exported symbol '" +
                                                            *import.Symbol + "'",
                                                        import.Location);
                    }
                    mModuleUsing[module_path].push_back(*match);
                } else {
                    // using module; bring all exports
                    for (const auto &exp : imported_exports) {
                        mModuleUsing[module_path].push_back(exp);
                    }
                }
            } else {
                // import: can export if marked
                if (import.IsFileImport) {
                    // import tests::foo; (foo is a file)  treat like whole-module import
                    if (import.Exported) {
                        for (const auto &exp : imported_exports) {
                            mModuleExports[module_path].push_back(exp);
                        }
                    }
                    // no symbol lookup; we just let the import exist for resolution
                } else if (import.Symbol.has_value()) {
                    // import module::symbol;
                    auto match = std::find_if(imported_exports.begin(), imported_exports.end(),
                                              [&](const ExportInfo &e) { return e.OriginalName == *import.Symbol; });
                    if (match == imported_exports.end()) {
                        throw Exceptions::SemanticError("Module '" + imported_path + "' has no exported symbol '" +
                                                            *import.Symbol + "'",
                                                        import.Location);
                    }
                    if (import.Exported) {
                        mModuleExports[module_path].push_back(*match);
                    }
                } else if (import.Exported) {
                    // export import module;
                    for (const auto &exp : imported_exports) {
                        mModuleExports[module_path].push_back(exp);
                    }
                }
            }
        }

        mParsedModules[module_path] = module;

        for (auto &decl : module.Declarations) {
            VisitDeclaration(decl, "", module_hash, module_path);
        }

        mEmissionOrder.push_back(module_path);
        mAnalyzedModules[module_path] = true;
    }

    AST::ASTModule SymanticAnalyser::LoadAndParseModule(const std::string &module_path) {
        std::string source = Common::GetScriptFromVFS(module_path, mVFS);
        auto tokens = Lexer::Lex(source, module_path);
        return Parser::ParseLexerOutput(tokens);
    }

    const Symbol *SymanticAnalyser::FindSymbol(const std::string &qualified_name,
                                               const std::string &module_path) const {
        if (!module_path.empty()) {
            auto module = mModuleSymbols.find(module_path);
            return module == mModuleSymbols.end() ? nullptr : module->second.Find(qualified_name);
        }

        for (const auto &path : mEmissionOrder) {
            auto module = mModuleSymbols.find(path);
            if (module != mModuleSymbols.end()) {
                if (const Symbol *symbol = module->second.Find(qualified_name)) {
                    return symbol;
                }
            }
        }
        return nullptr;
    }

    const Symbol *SymanticAnalyser::ResolveSymbol(const std::string &module_path,
                                                  const std::string &qualified_name) const {
        // first check local symbols
        if (const Symbol *local = FindSymbol(qualified_name, module_path)) {
            return local;
        }

        // check symbols brought in via 'using'
        auto using_it = mModuleUsing.find(module_path);
        if (using_it != mModuleUsing.end()) {
            for (const auto &export_info : using_it->second) {
                const std::string exported_name = export_info.Namespace.empty()
                                                      ? export_info.OriginalName
                                                      : export_info.Namespace + "::" + export_info.OriginalName;
                if (qualified_name == exported_name || qualified_name == export_info.OriginalName) {
                    if (const Symbol *symbol = FindSymbol(export_info.OriginalName, export_info.Modulepath)) {
                        return symbol;
                    }
                    if (const Symbol *symbol = FindSymbol(exported_name, export_info.Modulepath)) {
                        return symbol;
                    }
                }
            }
        }

        // then check imports
        auto parsed = mParsedModules.find(module_path);
        if (parsed == mParsedModules.end()) {
            return nullptr;
        }

        for (const auto &import : parsed->second.Imports) {
            if (import.IsUsing)
                continue; // already handled

            const std::string imported_path = Common::Import2Path(import, const_cast<VFS::VFS &>(mVFS));
            auto exports = mModuleExports.find(imported_path);
            if (exports == mModuleExports.end()) {
                continue;
            }

            for (const ExportInfo &export_info : exports->second) {
                if (!import.IsFileImport && import.Symbol && export_info.OriginalName != *import.Symbol) {
                    continue;
                }

                const std::string exported_name = export_info.Namespace.empty()
                                                      ? export_info.OriginalName
                                                      : export_info.Namespace + "::" + export_info.OriginalName;

                // Build the full module path from import.Path
                std::string full_module_path;
                for (size_t i = 0; i < import.Path.size(); ++i) {
                    if (i > 0)
                        full_module_path += "::";
                    full_module_path += import.Path[i];
                }

                const std::string module_qualified_name = full_module_path + "::" + export_info.OriginalName;

                if (qualified_name == exported_name || qualified_name == export_info.OriginalName ||
                    qualified_name == module_qualified_name) {

                    if (const Symbol *symbol = FindSymbol(export_info.OriginalName, export_info.Modulepath)) {
                        return symbol;
                    }
                    if (const Symbol *symbol = FindSymbol(exported_name, export_info.Modulepath)) {
                        return symbol;
                    }
                }
            }
        }

        return nullptr;
    }

    std::vector<const Symbol *> SymanticAnalyser::ResolveOverloads(const std::string &module_path,
                                                                   const std::string &qualified_name) const {
    std::vector<const Symbol *> result;
    std::unordered_set<const Symbol *> seen;  // prevent duplicates
    
    auto add = [&](const std::string &name, const std::string &path) {
        auto module = mModuleSymbols.find(path);
        
        if (module == mModuleSymbols.end())
            return;
        
        auto overloads = module->second.FindOverloads(name);
        
        for (const auto *symbol : overloads) {
            if (symbol->Kind == AST::ASTDeclaration::Kind::Function && seen.insert(symbol).second) {
                result.push_back(symbol);
            }
        }
    };
    
    add(qualified_name, module_path);
    
    if (!result.empty())
        return result;
    
    auto using_it = mModuleUsing.find(module_path);
    
    if (using_it != mModuleUsing.end()) {
        for (const auto &export_info : using_it->second) {
            
            std::string exported_name = export_info.Namespace.empty()
            ? export_info.OriginalName
            : export_info.Namespace + "::" + export_info.OriginalName;
            if (qualified_name == export_info.OriginalName || qualified_name == exported_name) {
                add(exported_name, export_info.Modulepath);
            }
        }
    }
    
    if (!result.empty())
        return result;
    
    auto parsed = mParsedModules.find(module_path);
    
    if (parsed == mParsedModules.end())
        return result;
    
    // debug loop show imports and exports
    for (const auto &import : parsed->second.Imports) {
        const std::string imported_path = Common::Import2Path(import, const_cast<VFS::VFS &>(mVFS));
        
        CE_LOG(LogLevel::Debug,
                "Import {} -> {}",
                import.Path.empty() ? "empty" : import.Path[0],
                imported_path);
        
        auto exports = mModuleExports.find(imported_path);
        
        if (exports == mModuleExports.end()) {
            CE_LOG(LogLevel::Debug, "No exports found");
            continue;
        }
        
        for (const auto &e : exports->second) {
            CE_LOG(LogLevel::Debug,
                    "Export: {}::{}",
                    e.Namespace,
                    e.OriginalName);
        }
    }
    
    for (const auto &import : parsed->second.Imports) {
        if (import.IsUsing)
            continue;
        
        const std::string imported_path = Common::Import2Path(import, const_cast<VFS::VFS &>(mVFS));
        
        auto exports = mModuleExports.find(imported_path);
        
        if (exports == mModuleExports.end())
            continue;
        
        for (const auto &export_info : exports->second) {
            if (export_info.Type != AST::ASTDeclaration::Kind::Function)
                continue;
            
            if (!import.IsFileImport && import.Symbol && export_info.OriginalName != *import.Symbol) {
                continue;
            }
            
            std::string exported_name = export_info.Namespace.empty()
            ? export_info.OriginalName
            : export_info.Namespace + "::" + export_info.OriginalName;
            
            std::string module_qualified_name;
            
            for (size_t i = 0; i < import.Path.size(); ++i) {
                if (i > 0)
                    module_qualified_name += "::";
                
                module_qualified_name += import.Path[i];
            }
            
            if (!module_qualified_name.empty())
                module_qualified_name += "::";
            
            module_qualified_name += export_info.OriginalName;
            
            if (qualified_name == export_info.OriginalName ||
                qualified_name == exported_name ||
                qualified_name == module_qualified_name) {
                
                CE_LOG(LogLevel::Debug, "Found matching export, calling add({}, {})", 
                        exported_name, export_info.Modulepath);
                
                add(exported_name, export_info.Modulepath);
            
            CE_LOG(LogLevel::Debug, "After add, result size: {}", result.size());
                }
        }
    }
    
    return result;
}

    const Symbol *SymanticAnalyser::ResolveFunction(const std::string &module_path, const std::string &name,
                                                    const std::vector<AST::ASTTypeRef> &arguments) const {
        auto overloads = ResolveOverloads(module_path, name);
        
        CE_LOG(LogLevel::Debug, "[Semantic Parser] Resolving: {}", name);

        CE_LOG(LogLevel::Debug, "Overloads found: {}", overloads.size());
        for (auto *x : overloads) {
            CE_LOG(LogLevel::Debug, "candidate: {}", x->QualifiedName);
        }
        
        const Symbol *match = nullptr;

        for (const auto *symbol : overloads) {
            const auto &parameters = symbol->Signature.Parameters;

            if (parameters.size() != arguments.size())
                continue;

            bool valid = true;

            for (size_t i = 0; i < parameters.size(); ++i) {
                const auto &expected = parameters[i];
                const auto &actual = arguments[i];

                std::string expected_name = StripNamespace(expected.Name);
                std::string actual_name = StripNamespace(actual.Name);

                if (actual_name == "null") {
                    if (!expected.IsHandle) {
                        valid = false;
                        break;
                    }
                    continue; // skip rest of checks for null matching a handle
                }

                if (expected_name != actual_name) {
                    valid = false;
                    break;
                }

                if (expected.IsConst != actual.IsConst) {
                    valid = false;
                    break;
                }

                if (expected.IsHandle != actual.IsHandle) {
                    valid = false;
                    break;
                }

                if (expected.IsReference != actual.IsReference) {
                    valid = false;
                    break;
                }

                if (expected.ArrayDepth != actual.ArrayDepth) {
                    valid = false;
                    break;
                }
            }

            if (valid) {
                if (match != nullptr) {
                    // ambiguous overload
                    return nullptr;
                }

                match = symbol;
            }
        }

        return match;
    }

    AST::ASTTypeRef SymanticAnalyser::GetGlobalType(const Symbol *symbol) const {
        auto mod_it = mParsedModules.find(symbol->SourceModule);
        if (mod_it != mParsedModules.end()) {
            const auto *decl = FindDeclarationInModule(mod_it->second, symbol->QualifiedName);
            if (decl && decl->Type == AST::ASTDeclaration::Kind::Global) {
                const auto &global = std::get<AST::ASTGlobal>(decl->Data);
                return global.Type;
            }
        }
        return {};
    }

    AST::ASTTypeRef SymanticAnalyser::GetMemberType(const std::string &class_name, const std::string &member_name,
                                                    const std::string &module_path) const {
        // 1. Check hardcoded engine types first
        std::string clean_class = StripNamespace(class_name);
        if (clean_class == "Colour") {
            if (member_name == "r" || member_name == "g" || member_name == "b" || member_name == "a") {
                AST::ASTTypeRef t;
                t.Name = "uint8";
                return t;
            }
        } else if (clean_class == "Vec3") {
            if (member_name == "x" || member_name == "y" || member_name == "z") {
                AST::ASTTypeRef t;
                t.Name = "float";
                return t;
            }
        } else if (clean_class == "Transform3D") {
            if (member_name == "position" || member_name == "rotation" || member_name == "scale") {
                AST::ASTTypeRef t;
                t.Name = "CE::Graphics::ThreeD::Vec3";
                return t;
            }
        } else if (clean_class == "Camera3D") {
            if (member_name == "position" || member_name == "target" || member_name == "up") {
                AST::ASTTypeRef t;
                t.Name = "CE::Graphics::ThreeD::Vec3";
                return t;
            } else if (member_name == "fov" || member_name == "nearClip" || member_name == "farClip") {
                AST::ASTTypeRef t;
                t.Name = "float";
                return t;
            } else if (member_name == "projection") {
                AST::ASTTypeRef t;
                t.Name = "CE::Graphics::ThreeD::CameraProjection";
                return t;
            }
        } else if (clean_class == "AudioEffect") {
            if (member_name == "enabled") {
                AST::ASTTypeRef t;
                t.Name = "bool";
                return t;
            } else if (member_name == "type") {
                AST::ASTTypeRef t;
                t.Name = "CE::Audio::AudioEffectType";
                return t;
            } else if (member_name == "wetMix" || member_name == "roomSize" || member_name == "damping") {
                AST::ASTTypeRef t;
                t.Name = "float";
                return t;
            }
        }

        // 2. Check script-defined classes
        const Symbol *class_sym = ResolveSymbol(module_path, class_name);
        if (class_sym && class_sym->Kind == AST::ASTDeclaration::Kind::Type) {
            auto mod_it = mParsedModules.find(class_sym->SourceModule);
            if (mod_it != mParsedModules.end()) {
                const auto *decl = FindDeclarationInModule(mod_it->second, class_sym->QualifiedName);
                if (decl && decl->Type == AST::ASTDeclaration::Kind::Type) {
                    const auto &ast_type = std::get<AST::ASTType>(decl->Data);
                    const auto &tokens = ast_type.Body;
                    for (size_t i = 0; i < tokens.size(); ++i) {
                        size_t j = i;
                        bool is_const = false;
                        if (tokens[j].Type == Lexer::Token::TokenType::KeywordConst) {
                            is_const = true;
                            j++;
                        }
                        if (j < tokens.size() && tokens[j].Type == Lexer::Token::TokenType::Identifier) {
                            std::string type_name = tokens[j].Value;
                            j++;
                            while (j + 1 < tokens.size() &&
                                   tokens[j].Type == Lexer::Token::TokenType::ScopeResolution &&
                                   tokens[j + 1].Type == Lexer::Token::TokenType::Identifier) {
                                type_name += "::" + tokens[j + 1].Value;
                                j += 2;
                            }
                            bool is_handle = false;
                            bool is_ref = false;
                            uint32_t array_depth = 0;
                            while (j < tokens.size()) {
                                if (tokens[j].Type == Lexer::Token::TokenType::Handle) {
                                    is_handle = true;
                                    j++;
                                } else if (tokens[j].Type == Lexer::Token::TokenType::Reference) {
                                    is_ref = true;
                                    j++;
                                } else if (tokens[j].Type == Lexer::Token::TokenType::OpenBracket) {
                                    if (j + 1 < tokens.size() &&
                                        tokens[j + 1].Type == Lexer::Token::TokenType::CloseBracket) {
                                        array_depth++;
                                        j += 2;
                                    } else {
                                        break;
                                    }
                                } else {
                                    break;
                                }
                            }
                            if (j < tokens.size() && tokens[j].Type == Lexer::Token::TokenType::Identifier) {
                                std::string name = tokens[j].Value;
                                j++;
                                if (j < tokens.size() && (tokens[j].Type == Lexer::Token::TokenType::Semicolon ||
                                                          tokens[j].Type == Lexer::Token::TokenType::Assignment ||
                                                          tokens[j].Type == Lexer::Token::TokenType::OpenParen)) {
                                    if (tokens[j].Type != Lexer::Token::TokenType::OpenParen) {
                                        if (name == member_name) {
                                            AST::ASTTypeRef t;
                                            t.Name = type_name;
                                            t.IsConst = is_const;
                                            t.IsHandle = is_handle;
                                            t.IsReference = is_ref;
                                            t.ArrayDepth = array_depth;
                                            return t;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return {};
    }

    const Symbol *SymanticAnalyser::FindDeclarationSymbol(const std::string &qualified_name,
                                                          const std::string &module_path,
                                                          const AST::ASTDeclaration &decl) const {

        auto module = mModuleSymbols.find(module_path);

        if (module == mModuleSymbols.end())
            return nullptr;

        auto overloads = module->second.FindOverloads(qualified_name);

        if (decl.Type != AST::ASTDeclaration::Kind::Function) {
            for (const auto *symbol : overloads) {
                if (symbol->Kind == decl.Type)
                    return symbol;
            }

            return nullptr;
        }

        const auto &function = std::get<AST::ASTFunction>(decl.Data);

        std::string signature_hash = GenerateSignatureHash(function);

        for (const auto *symbol : overloads) {
            if (symbol->InternalName.find(signature_hash) != std::string::npos)
                return symbol;
        }

        return nullptr;
    }
} // namespace CE::Scripting::Impl::Semantics
