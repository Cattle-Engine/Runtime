#include "engine/scripting/private/modules.hpp"

#include "engine/common/utils/hasher.hpp"
#include "engine/scripting/private/common.hpp"
#include "engine/scripting/private/generator.hpp"
#include "engine/scripting/private/lexer.hpp"
#include "engine/scripting/private/parser.hpp"
#include "engine/scripting/private/semantics.hpp"

namespace CE::Scripting::Impl {
    namespace {
        void CollectDiagnosticSymbolNames(const AST::ASTDeclaration& declaration, const std::string& module_path,
                                          const std::string& name_space, const Semantics::SymanticAnalyser& analyser,
                                          std::unordered_map<std::string, std::string>& names,
                                          std::vector<GeneratedSymbolInfo>& generated_symbols) {
            if (declaration.Type == AST::ASTDeclaration::Kind::Namespace) {
                const auto& name_space_declaration = *std::get<std::shared_ptr<AST::ASTNamespace>>(declaration.Data);
                const std::string nested = name_space.empty() ? name_space_declaration.Name
                                                               : name_space + "::" + name_space_declaration.Name;
                for (const auto& child : name_space_declaration.Declarations) {
                    CollectDiagnosticSymbolNames(child, module_path, nested, analyser, names, generated_symbols);
                }
                return;
            }

            if (declaration.Type == AST::ASTDeclaration::Kind::Raw) {
                return;
            }

            const std::string qualified = name_space.empty() ? declaration.Name : name_space + "::" + declaration.Name;
            if (const auto* symbol = analyser.FindDeclarationSymbol(qualified, module_path, declaration)) {
                names.emplace(symbol->InternalName, qualified);
                generated_symbols.push_back({symbol->InternalName, qualified, qualified,
                                             symbol->Kind == AST::ASTDeclaration::Kind::Function ? "function"
                                             : symbol->Kind == AST::ASTDeclaration::Kind::Global ? "global"
                                             : symbol->Kind == AST::ASTDeclaration::Kind::Type ? "type"
                                                                                               : "symbol"});
            }
        }
    } // namespace

    std::string MangledSymbolInfo::GenerateMangledName() const {
        switch (Type) {
        case SymbolType::Function:
            return "__ce_mod_f_" + std::to_string(ModuleHash) + "_" + std::to_string(NameSpaceHash) + "_" +
                   std::to_string(SymbolHash) + "_" + std::to_string(SignatureHash);

        case SymbolType::Global:
            return "__ce_mod_g_" + std::to_string(ModuleHash) + "_" + std::to_string(NameSpaceHash) + "_" +
                   std::to_string(SymbolHash) + "_" + std::to_string(TypeHash);

        case SymbolType::Type:
            return "__ce_mod_t_" + std::to_string(ModuleHash) + "_" + std::to_string(NameSpaceHash) + "_" +
                   std::to_string(SymbolHash);

        case SymbolType::Internal:
            return "__ce_mod_i_" + std::to_string(ModuleHash) + "_" + std::to_string(NameSpaceHash) + "_" +
                   std::to_string(SymbolHash) + "_" + std::to_string(SignatureHash);
        }
        return {};
    }

    ModuleImporter::ModuleImporter(::CE::Common::FS::VFS::VFS& vfs) : mVFS(vfs) {}

    std::vector<GeneratedScriptSection> ModuleImporter::LoadFile(const std::string& filepath) {
        if (!mVFS.FileExists(filepath.c_str())) {
            throw std::runtime_error("File not found: " + filepath);
        }
        mLoadModules.clear();
        mDiagnosticSymbolNames.clear();
        mGeneratedSymbols.clear();
        Semantics::SymanticAnalyser analyser(mVFS);
        AST::ASTModule root = Parser::ParseLexerOutput(Lexer::Lex(Common::GetScriptFromVFS(filepath, mVFS), filepath));
        analyser.CheckModule(root, filepath);
        for (const auto& [module_path, module] : analyser.GetParsedModules()) {
            for (const auto& declaration : module.Declarations) {
                CollectDiagnosticSymbolNames(declaration, module_path, "", analyser, mDiagnosticSymbolNames,
                                             mGeneratedSymbols);
            }
        }
        mEntrypoints.clear();
        for (const std::string& source_name : {std::string("main"), std::string("update"), std::string("imgui")}) {
            if (const auto* symbol = analyser.FindSymbol(source_name, filepath)) {
                if (symbol->Kind == AST::ASTDeclaration::Kind::Function) {
                    mEntrypoints.emplace(source_name, symbol->InternalName);
                }
            }
        }
        Codegen::Generator generator(analyser);
        return generator.GenerateScriptSections(analyser.GetEmissionOrder(), analyser.GetParsedModules());
    }

    ModuleInfo ModuleImporter::LoadModule(const std::string& name) {
        ModuleInfo info;
        info.Name = name;
        const std::string source = Common::GetScriptFromVFS(name, mVFS);
        if (source.empty() && !mVFS.FileExists(name.c_str())) {
            throw std::runtime_error("File not found: " + name);
        }
        info.Hash = Utils::Hash64(source);
        return info;
    }

    std::string ModuleImporter::GetGeneratedEntrypoint(const std::string& source_name) const {
        auto entrypoint = mEntrypoints.find(source_name);
        return entrypoint == mEntrypoints.end() ? std::string{} : entrypoint->second;
    }
} // namespace CE::Scripting::Impl
