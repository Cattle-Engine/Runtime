#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "engine/scripting/private/modules.hpp"

namespace CE::Scripting::Impl::AST {
    struct ASTImport {
        std::string Module;
        std::optional<std::string> Symbol;

        SourceLocation Location;
    };

    struct ASTTypeRef {
        std::string Name;

        bool IsConst = false;
        bool IsReference = false;
        bool IsHandle = false;
        bool IsArray = false;
        bool IsAuto = false;
    };

    struct ASTParameter {
        ASTTypeRef Type;
        std::string Name;
    };

    struct ASTFunction {
        std::string Name;
        ASTTypeRef ReturnType;

        std::vector<ASTParameter> Parameters;

        std::string Body;
    };

    struct ASTGlobal {
        ASTTypeRef Type;
        std::string Name;

        std::string Initializer;
    };

    struct ASTType {
        std::string Name;

        std::string Body;
    };

    struct ASTNamespace;

    struct ASTDeclaration {
        enum class Kind {
            Function,
            Global,
            Type,
            Namespace
        };

        Kind Kind;
        bool Exported = false;

        std::string Name;
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
}