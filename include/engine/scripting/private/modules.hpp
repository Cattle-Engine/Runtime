#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/common/fs/vfs.hpp"

namespace CE::Scripting::Impl {   
    struct SourceLocation {
        std::string File = "";
        uint32_t Line = 0;
        uint32_t Column = 0;
    };
    
    struct MangledSymbolInfo {
        enum class SymbolType {
            Function,
            Global,
            Type,
            Internal
        };

        bool Exported = false;
        SymbolType Type = SymbolType::Internal;
        SourceLocation Location;

        std::string ModuleName = "";
        std::string NameSpace = "";
        std::string Name;

        std::string Signature = "";
        std::string ReturnType = "";

        uint64_t ModuleHash = 0;
        uint64_t NameSpaceHash = 0;
        uint64_t SymbolHash = 0;
        uint64_t SignatureHash = 0;
        uint64_t TypeHash = 0;

        std::string MangledName = "";

        std::string GenerateMangledName() const;
    };

    struct ModuleInfo {
        enum class ModuleState {
            NotLoaded,
            Loaded
        };

        std::string Name;
        uint64_t Hash = 0;

        std::vector<std::string> Imports;
        std::vector<MangledSymbolInfo> Symbols;

        std::string GeneratedCode;
    };
    
    class ModuleImporter {
        public:
            ModuleImporter(VFS::VFS& vfs);

            /**
             * @brief Used to load 1 script file and resolve it's imports
             * @return Returns a std::string with everything resolved to be built to bytecode, throws if an error happened
             */ 
            std::string LoadFile(const std::string& filepath);
            ModuleInfo LoadModule(const std::string& name);
            std::string GetGeneratedEntrypoint(const std::string& source_name) const;
        private:
            std::string GenerateCombinedScripts();

            VFS::VFS& mVFS;
            std::vector<std::string> mLoadModules;
            std::unordered_map<std::string, std::string> mEntrypoints;
    };
}
