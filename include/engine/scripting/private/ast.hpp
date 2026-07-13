#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <cstdint>

#include "engine/scripting/private/modules.hpp"
#include "engine/scripting/private/lexer.hpp"

namespace CE::Scripting::Impl::AST {
    struct ASTImport {
        std::string Module;
        std::vector<std::string> Path;
        std::optional<std::string> Symbol;
        bool Exported = false;
        
        SourceLocation Location;
    };

    struct ASTTypeRef {
        std::string Name;

        bool IsConst = false;
        bool IsReference = false;
        bool IsHandle = false;
        uint32_t ArrayDepth = 0;
        bool IsAuto = false;
    };

    struct ASTParameter {
        ASTTypeRef Type;
        std::string Name;
        // AngelScript parameter direction: in, out, or inout.
        std::string Direction;
    };
    
    struct ASTLocalVariable {
        ASTTypeRef Type;
        std::string Name;
    };
    
    struct ASTFunction {
        std::string Name;
        ASTTypeRef ReturnType;
        
        std::vector<ASTParameter> Parameters;
        std::vector<Lexer::Token> Body;
        std::vector<ASTLocalVariable> LocalVariables; // Exactly what you need
    };

    struct ASTGlobal {
        ASTTypeRef Type;
        std::string Name;

        std::vector<Lexer::Token> Initializer;
    };

    struct ASTType {
        std::string Name;

        std::vector<Lexer::Token> Body;
    };

    struct ASTNamespace;

    struct ASTDeclaration {
        enum class Kind {
            Function,
            Global,
            Type,
            Namespace
        };

        Kind Type;
        bool Exported = false;

        std::string Name;
        SourceLocation Location;
        std::string NameSpace;

        std::variant<
            ASTFunction,
            ASTGlobal,
            ASTType,
            std::shared_ptr<ASTNamespace>
        > Data;
    };

    struct ASTNamespace {
        std::string Name;

        std::vector<ASTDeclaration> Declarations;
    };

    struct ASTModule {
        std::vector<ASTImport> Imports;
        std::vector<ASTDeclaration> Declarations;
    };
    
    /**
     * @brief Hashes a module deterministly
     * 
     * This is mostly used by the ast to generate the internal name
     * 
     * @return Returns a uint64_t of the hashed model
     */
    uint64_t HashModule(const ASTModule& module);
}
