#include "ce_common/as_compiler.hpp"

#include <filesystem>
#include <fstream>
#include <future>
#include <sstream>
#include <utility>

#include "ce_common/detail/hasher.hpp"
#include "ce_common/as_compiler/common.hpp"
#include "ce_common/as_compiler/generator.hpp"
#include "ce_common/as_compiler/lexer.hpp"
#include "ce_common/as_compiler/parser.hpp"
#include "ce_common/as_compiler/semantics.hpp"

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
                std::string kind = "symbol";
                if (symbol->Kind == AST::ASTDeclaration::Kind::Function) {
                    kind = "function";
                } else if (symbol->Kind == AST::ASTDeclaration::Kind::Global) {
                    kind = "global";
                } else if (symbol->Kind == AST::ASTDeclaration::Kind::Type) {
                    kind = "type";
                }

                generated_symbols.push_back({symbol->InternalName, qualified, qualified, kind});
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

    ModuleImporter::ModuleImporter(std::vector<FS::path> import_paths, Preprocessor& p) : mImportPaths(std::move(import_paths)), mPreprocessor(p) {}

    FS::path ModuleImporter::ResolvePath(const FS::path& path) const {
        if (path.is_absolute())
            return path;

        for (const auto& root : mImportPaths) {
            const auto candidate = root / path;
            if (std::filesystem::exists(candidate))
                return candidate;
        }

        if (std::filesystem::exists(path))
            return path;

        return path;
    }

    std::vector<GeneratedScriptSection> ModuleImporter::LoadFile(const std::string& filepath) {
        const std::filesystem::path resolved = std::filesystem::path(filepath);
        const std::filesystem::path absolute = ResolvePath(resolved);
        if (!std::filesystem::exists(absolute)) {
            throw std::runtime_error("File not found: " + filepath);
        }

        mLoadModules.clear();
        mDiagnosticSymbolNames.clear();
        mGeneratedSymbols.clear();

        const std::string source = Common::GetScriptFromFS(absolute.string());
        std::string processed_source;

        if (!mPreprocessor.Process(source, processed_source)) {
            throw std::runtime_error(
                "Preprocessing failed for module: " + filepath
            );
        }

        AST::ASTModule root = Parser::ParseLexerOutput(
            Lexer::Lex(processed_source, filepath)
        );

        Semantics::SymanticAnalyser analyser(mPreprocessor, mImportPaths);
        analyser.SetModuleSource(absolute.string(), processed_source);
        analyser.CheckModule(root, absolute.string());
        for (const auto& [module_path, module] : analyser.GetParsedModules()) {
            for (const auto& declaration : module.Declarations) {
                CollectDiagnosticSymbolNames(declaration, module_path, "", analyser, mDiagnosticSymbolNames,
                                             mGeneratedSymbols);
            }
        }
        mEntrypoints.clear();
        for (const std::string& source_name : {std::string("main"), std::string("update"), std::string("imgui")}) {
            if (const auto* symbol = analyser.FindSymbol(source_name, absolute.string())) {
                if (symbol->Kind == AST::ASTDeclaration::Kind::Function) {
                    mEntrypoints.emplace(source_name, symbol->InternalName);
                }
            }
        }

        Codegen::Generator generator(analyser);

        auto sections = generator.GenerateScriptSections(
            analyser.GetEmissionOrder(),
            analyser.GetParsedModules());

        const auto& module_sources = analyser.GetModuleSources();

        for (auto& section : sections) {
            const auto it = module_sources.find(section.Name);
            if (it != module_sources.end()) {
                section.Source = it->second;
            }
        }

        return sections;
    }

    ModuleInfo ModuleImporter::LoadModule(const std::string& name) {
        ModuleInfo info;
        info.Name = name;

        const std::filesystem::path resolved = std::filesystem::path(name);
        const std::filesystem::path absolute = ResolvePath(resolved);
        const std::string source = Common::GetScriptFromFS(absolute.string());
        if (source.empty() && !std::filesystem::exists(absolute)) {
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
