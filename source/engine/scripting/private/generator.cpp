#include "engine/scripting/private/generator.hpp"

#include <unordered_set>

namespace CE::Scripting::Impl::Codegen {
    namespace {
        bool IsIdentifier(const Lexer::Token& token) {
            return token.Type == Lexer::Token::TokenType::Identifier;
        }

        bool NeedsSpace(const Lexer::Token& previous, const Lexer::Token& current) {
            const std::string& a = previous.Value;
            const std::string& b = current.Value;
            if (b == ";" || b == "," || b == ")" || b == "]" || b == "." || b == "::" ||
                a == "(" || a == "[" || a == "." || a == "::" || a == "@") return false;
            if (b == "(" || b == "[" || a == "{") return false;
            if (a == "}" || b == "{") return true;
            const bool a_word = IsIdentifier(previous) || previous.Type == Lexer::Token::TokenType::Number ||
                                previous.Type == Lexer::Token::TokenType::String;
            const bool b_word = IsIdentifier(current) || current.Type == Lexer::Token::TokenType::Number ||
                                current.Type == Lexer::Token::TokenType::String;
            return a_word && b_word;
        }
    }

    std::string Generator::JoinTokens(const std::vector<Lexer::Token>& tokens) {
        std::string result;
        for (const auto& token : tokens) {
            if (token.Type == Lexer::Token::TokenType::EndOfFile) continue;
            if (!result.empty() && NeedsSpace(tokens[&token - tokens.data() - 1], token)) result += ' ';
            result += token.Value;
        }
        return result;
    }

    std::string Generator::EmitTypeRef(const AST::ASTTypeRef& type, const std::string& module_path,
                                       const std::string& name_space) const {
        std::string name = type.Name;
        if (!type.IsAuto) {
            const std::string qualified = name.find("::") != std::string::npos || name_space.empty()
                ? name : name_space + "::" + name;
            if (const auto* symbol = mAnalyser.ResolveSymbol(module_path, qualified)) {
                if (symbol->Kind == AST::ASTDeclaration::Kind::Type) name = symbol->InternalName;
            } else if (const auto* symbol = mAnalyser.ResolveSymbol(module_path, name)) {
                if (symbol->Kind == AST::ASTDeclaration::Kind::Type) name = symbol->InternalName;
            }
        }
        std::string result = type.IsConst ? "const " : "";
        result += type.IsAuto ? "auto" : name;
        for (uint32_t i = 0; i < type.ArrayDepth; ++i) result += "[]";
        if (type.IsHandle) result += '@';
        if (type.IsReference) result += '&';
        return result;
    }

    std::vector<Lexer::Token> Generator::RewriteBody(const std::vector<Lexer::Token>& tokens,
                                                       const AST::ASTFunction* function,
                                                       const std::string& module_path,
                                                       const std::string& name_space) const {
        std::unordered_set<std::string> shadowed;
        if (function) {
            for (const auto& parameter : function->Parameters) shadowed.insert(parameter.Name);
            for (const auto& local : function->LocalVariables) shadowed.insert(local.Name);
        }
        std::vector<Lexer::Token> result;
        for (size_t i = 0; i < tokens.size(); ++i) {
            Lexer::Token token = tokens[i];
            if (!IsIdentifier(token) || shadowed.contains(token.Value) ||
                (i > 0 && tokens[i - 1].Value == ".")) {
                result.push_back(std::move(token));
                continue;
            }

            std::string candidate = token.Value;
            size_t end = i;
            while (end + 2 < tokens.size() && tokens[end + 1].Type == Lexer::Token::TokenType::ScopeResolution &&
                   IsIdentifier(tokens[end + 2])) {
                candidate += "::" + tokens[end + 2].Value;
                end += 2;
            }
            const std::string scoped = candidate.find("::") == std::string::npos && !name_space.empty()
                ? name_space + "::" + candidate : candidate;
            const auto* symbol = mAnalyser.ResolveSymbol(module_path, scoped);
            if (!symbol && scoped != candidate) symbol = mAnalyser.ResolveSymbol(module_path, candidate);
            if (symbol && symbol->Kind != AST::ASTDeclaration::Kind::Namespace) {
                token.Value = symbol->InternalName;
                result.push_back(std::move(token));
                i = end;
            } else {
                result.push_back(std::move(token));
            }
        }
        return result;
    }

    std::string Generator::EmitDeclaration(const AST::ASTDeclaration& declaration,
                                           const std::string& module_path,
                                           const std::string& name_space) const {
        if (declaration.Type == AST::ASTDeclaration::Kind::Namespace) {
            std::string result;
            const auto& ns = *std::get<std::shared_ptr<AST::ASTNamespace>>(declaration.Data);
            const std::string nested = name_space.empty() ? ns.Name : name_space + "::" + ns.Name;
            for (const auto& child : ns.Declarations) result += EmitDeclaration(child, module_path, nested);
            return result;
        }
        const std::string qualified = name_space.empty() ? declaration.Name : name_space + "::" + declaration.Name;
        const auto* symbol = mAnalyser.FindSymbol(qualified, module_path);
        if (!symbol) return {};
        if (declaration.Type == AST::ASTDeclaration::Kind::Function) {
            const auto& function = std::get<AST::ASTFunction>(declaration.Data);
            std::string result = EmitTypeRef(function.ReturnType, module_path, name_space) + " " + symbol->InternalName + "(";
            for (size_t i = 0; i < function.Parameters.size(); ++i) {
                if (i) result += ", ";
                result += EmitTypeRef(function.Parameters[i].Type, module_path, name_space);
                if (!function.Parameters[i].Direction.empty()) result += function.Parameters[i].Direction;
                result += " " + function.Parameters[i].Name;
            }
            return result + ")" + JoinTokens(RewriteBody(function.Body, &function, module_path, name_space)) + "\n";
        }
        if (declaration.Type == AST::ASTDeclaration::Kind::Global) {
            const auto& global = std::get<AST::ASTGlobal>(declaration.Data);
            std::string result = EmitTypeRef(global.Type, module_path, name_space) + " " + symbol->InternalName;
            if (!global.Initializer.empty()) result += " = " + JoinTokens(RewriteBody(global.Initializer, nullptr, module_path, name_space));
            return result + ";\n";
        }
        const auto& type = std::get<AST::ASTType>(declaration.Data);
        return "class " + symbol->InternalName + " {" + JoinTokens(RewriteBody(type.Body, nullptr, module_path, name_space)) + "};\n";
    }

    std::string Generator::GenerateMonoScript(const std::vector<std::string>& emission_order,
                                              const std::unordered_map<std::string, AST::ASTModule>& parsed_modules) const {
        std::string result;
        for (const auto& module_path : emission_order) {
            auto module = parsed_modules.find(module_path);
            if (module == parsed_modules.end()) continue;
            for (const auto& declaration : module->second.Declarations) {
                result += EmitDeclaration(declaration, module_path, "");
            }
        }
        return result;
    }
}
