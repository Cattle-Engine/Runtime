#include "engine/scripting/private/modules.hpp"
#include "engine/common/utils/hasher.hpp"

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

        ModuleInfo root = LoadModule(filepath);

        
    }

    ModuleInfo ModuleImporter::LoadModule(const std::string& name) {
        
    }
}