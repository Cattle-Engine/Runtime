#include "engine/scripting/private/modules.hpp"
#include "engine/common/utils/hasher.hpp"
#include "engine/scripting/private/common.hpp"
#include "engine/scripting/private/generator.hpp"
#include "engine/scripting/private/lexer.hpp"
#include "engine/scripting/private/parser.hpp"
#include "engine/scripting/private/semantics.hpp"

namespace CE::Scripting::Impl {
    std::string MangledSymbolInfo::GenerateMangledName() const {
        switch (Type) {
            case SymbolType::Function:
                return "__ce_mod_f_" +
                    std::to_string(ModuleHash) + "_" +
                    std::to_string(NameSpaceHash) + "_" +
                    std::to_string(SymbolHash) + "_" +
                    std::to_string(SignatureHash);

            case SymbolType::Global:
                return "__ce_mod_g_" +
                    std::to_string(ModuleHash) + "_" +
                    std::to_string(NameSpaceHash) + "_" +
                    std::to_string(SymbolHash) + "_" +
                    std::to_string(TypeHash);

            case SymbolType::Type:
                return "__ce_mod_t_" +
                    std::to_string(ModuleHash) + "_" +
                    std::to_string(NameSpaceHash) + "_" +
                    std::to_string(SymbolHash);

            case SymbolType::Internal:
                return "__ce_mod_i_" +
                    std::to_string(ModuleHash) + "_" +
                    std::to_string(NameSpaceHash) + "_" +
                    std::to_string(SymbolHash) + "_" +
                    std::to_string(SignatureHash);
        }
        return {};
    }

    ModuleImporter::ModuleImporter(VFS::VFS& vfs) : mVFS(vfs) {}

    std::string ModuleImporter::LoadFile(const std::string& filepath) {
        if (!mVFS.FileExists(filepath.c_str())) {
            throw std::runtime_error("File not found: " + filepath);
        }
        mLoadModules.clear();
        Semantics::SymanticAnalyser analyser(mVFS);
        AST::ASTModule root = Parser::ParseLexerOutput(
            Lexer::Lex(Common::GetScriptFromVFS(filepath, mVFS), filepath));
        analyser.CheckModule(root, filepath);
        mEntrypoints.clear();
        for (const std::string& source_name : {std::string("main"), std::string("update")}) {
            if (const auto* symbol = analyser.FindSymbol(source_name, filepath)) {
                if (symbol->Kind == AST::ASTDeclaration::Kind::Function) {
                    mEntrypoints.emplace(source_name, symbol->InternalName);
                }
            }
        }
        Codegen::Generator generator(analyser);
        return generator.GenerateMonoScript(analyser.GetEmissionOrder(), analyser.GetParsedModules());
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
}
