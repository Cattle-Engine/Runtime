#include "engine/scripting/private/generator.hpp"

#include <unordered_set>

namespace CE::Scripting::Impl::Codegen {
    namespace {
        bool IsIdentifier(const Lexer::Token &token) {
            return token.Type == Lexer::Token::TokenType::Identifier;
        }

        bool NeedsSpace(const Lexer::Token &previous, const Lexer::Token &current) {
            const std::string &a = previous.Value;
            const std::string &b = current.Value;
            if (b == ";" || b == "," || b == ")" || b == "]" || b == "." || b == "::" || a == "(" || a == "[" ||
                a == "." || a == "::" || a == "@")
                return false;
            if (b == "(" || b == "[" || a == "{")
                return false;
            if (a == "}" || b == "{")
                return true;
            const bool a_word = IsIdentifier(previous) || previous.Type == Lexer::Token::TokenType::Number ||
                                previous.Type == Lexer::Token::TokenType::String;
            const bool b_word = IsIdentifier(current) || current.Type == Lexer::Token::TokenType::Number ||
                                current.Type == Lexer::Token::TokenType::String;
            return a_word && b_word;
        }
    } // namespace

    std::string Generator::JoinTokens(const std::vector<Lexer::Token> &tokens) {
        std::string result;
        for (const auto &token : tokens) {
            if (token.Type == Lexer::Token::TokenType::EndOfFile)
                continue;
            if (!result.empty() && NeedsSpace(tokens[&token - tokens.data() - 1], token))
                result += ' ';
            result += token.Value;
        }
        return result;
    }

    std::string Generator::EmitTypeRef(const AST::ASTTypeRef &type, const std::string &module_path,
                                       const std::string &name_space) const {
        std::string name = type.Name;
        if (!type.IsAuto) {
            const std::string qualified =
                name.find("::") != std::string::npos || name_space.empty() ? name : name_space + "::" + name;
            if (const auto *symbol = mAnalyser.ResolveSymbol(module_path, qualified)) {
                if (symbol->Kind == AST::ASTDeclaration::Kind::Type)
                    name = symbol->InternalName;
            } else if (const auto *symbol = mAnalyser.ResolveSymbol(module_path, name)) {
                if (symbol->Kind == AST::ASTDeclaration::Kind::Type)
                    name = symbol->InternalName;
            }
        }
        std::string result = type.IsConst ? "const " : "";
        result += type.IsAuto ? "auto" : name;
        for (uint32_t i = 0; i < type.ArrayDepth; ++i)
            result += "[]";
        if (type.IsHandle)
            result += '@';
        if (type.IsReference)
            result += '&';
        return result;
    }

    std::vector<Lexer::Token> Generator::RewriteBody(const std::vector<Lexer::Token> &tokens,
                                                     const AST::ASTFunction *function, const std::string &module_path,
                                                     const std::string &name_space) const {
        std::unordered_set<std::string> shadowed;
        if (function) {
            for (const auto &parameter : function->Parameters)
                shadowed.insert(parameter.Name);
            for (const auto &local : function->LocalVariables)
                shadowed.insert(local.Name);
        }
        std::vector<Lexer::Token> result;
        for (size_t i = 0; i < tokens.size(); ++i) {
            Lexer::Token token = tokens[i];
            if (!IsIdentifier(token) || shadowed.contains(token.Value) || (i > 0 && tokens[i - 1].Value == ".")) {
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
                                           ? name_space + "::" + candidate
                                           : candidate;

            const Semantics::Symbol *symbol = nullptr;
            bool is_function_call = false;
            std::vector<AST::ASTTypeRef> arg_types;

            // Check if followed by '('
            if (end + 1 < tokens.size() && tokens[end + 1].Type == Lexer::Token::TokenType::OpenParen) {
                // Find matching close paren
                size_t p_depth = 0;
                size_t b_depth = 0;
                size_t br_depth = 0;
                size_t matching_close = 0;
                for (size_t k = end + 1; k < tokens.size(); ++k) {
                    if (tokens[k].Type == Lexer::Token::TokenType::OpenParen) p_depth++;
                    else if (tokens[k].Type == Lexer::Token::TokenType::CloseParen) {
                        p_depth--;
                        if (p_depth == 0) {
                            matching_close = k;
                            break;
                        }
                    } else if (tokens[k].Type == Lexer::Token::TokenType::OpenBracket) b_depth++;
                    else if (tokens[k].Type == Lexer::Token::TokenType::CloseBracket) b_depth--;
                    else if (tokens[k].Type == Lexer::Token::TokenType::OpenBrace) br_depth++;
                    else if (tokens[k].Type == Lexer::Token::TokenType::CloseBrace) br_depth--;
                }

                if (matching_close > 0) {
                    is_function_call = true;
                    // Parse the arguments inside the parenthesis
                    std::vector<std::vector<Lexer::Token>> call_args;
                    std::vector<Lexer::Token> current_arg;
                    p_depth = 0;
                    b_depth = 0;
                    br_depth = 0;
                    for (size_t k = end + 2; k < matching_close; ++k) {
                        const auto &tk = tokens[k];
                        if (tk.Type == Lexer::Token::TokenType::OpenParen) p_depth++;
                        else if (tk.Type == Lexer::Token::TokenType::CloseParen) p_depth--;
                        else if (tk.Type == Lexer::Token::TokenType::OpenBracket) b_depth++;
                        else if (tk.Type == Lexer::Token::TokenType::CloseBracket) b_depth--;
                        else if (tk.Type == Lexer::Token::TokenType::OpenBrace) br_depth++;
                        else if (tk.Type == Lexer::Token::TokenType::CloseBrace) br_depth--;

                        if (tk.Type == Lexer::Token::TokenType::Comma && p_depth == 0 && b_depth == 0 && br_depth == 0) {
                            call_args.push_back(current_arg);
                            current_arg.clear();
                        } else {
                            current_arg.push_back(tk);
                        }
                    }
                    if (!current_arg.empty()) {
                        call_args.push_back(current_arg);
                    }

                    for (const auto &ca : call_args) {
                        arg_types.push_back(InferExpressionType(ca, function, module_path, name_space));
                    }
                }
            }

            if (is_function_call) {
                symbol = mAnalyser.ResolveFunction(module_path, candidate, arg_types);
                if (!symbol && scoped != candidate) {
                    symbol = mAnalyser.ResolveFunction(module_path, scoped, arg_types);
                }
            }

            if (!symbol) {
                symbol = mAnalyser.ResolveSymbol(module_path, candidate);
                if (!symbol && scoped != candidate) {
                    symbol = mAnalyser.ResolveSymbol(module_path, scoped);
                }
            }

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

    AST::ASTTypeRef Generator::InferExpressionType(const std::vector<Lexer::Token> &tokens,
                                                   const AST::ASTFunction *function,
                                                   const std::string &module_path,
                                                   const std::string &name_space) const {
        if (tokens.empty())
            return {};

        // 1. If it's a single token:
        if (tokens.size() == 1) {
            const auto &t = tokens[0];
            if (t.Type == Lexer::Token::TokenType::Number) {
                AST::ASTTypeRef type;
                if (t.Value.find('.') != std::string::npos || t.Value.back() == 'f' || t.Value.back() == 'F') {
                    type.Name = "float";
                } else {
                    type.Name = "int";
                }
                return type;
            }
            if (t.Type == Lexer::Token::TokenType::String) {
                AST::ASTTypeRef type;
                type.Name = "string";
                return type;
            }
            if (t.Type == Lexer::Token::TokenType::Identifier) {
                if (t.Value == "true" || t.Value == "false") {
                    AST::ASTTypeRef type;
                    type.Name = "bool";
                    return type;
                }
                if (t.Value == "null") {
                    AST::ASTTypeRef type;
                    type.Name = "null";
                    return type;
                }
                // Check parameter
                if (function) {
                    for (const auto &param : function->Parameters) {
                        if (param.Name == t.Value)
                            return param.Type;
                    }
                    for (const auto &local : function->LocalVariables) {
                        if (local.Name == t.Value)
                            return local.Type;
                    }
                }
                // Check global variable
                std::string scoped = t.Value.find("::") == std::string::npos && !name_space.empty()
                                         ? name_space + "::" + t.Value
                                         : t.Value;
                const auto *symbol = mAnalyser.ResolveSymbol(module_path, t.Value);
                if (!symbol && scoped != t.Value)
                    symbol = mAnalyser.ResolveSymbol(module_path, scoped);

                if (symbol) {
                    if (symbol->Kind == AST::ASTDeclaration::Kind::Global) {
                        return mAnalyser.GetGlobalType(symbol);
                    }
                }
            }
        }

        // 2. Check if the entire token stream is a function call:
        size_t name_end = 0;
        while (name_end < tokens.size() &&
               (tokens[name_end].Type == Lexer::Token::TokenType::Identifier ||
                tokens[name_end].Type == Lexer::Token::TokenType::ScopeResolution)) {
            name_end++;
        }
        if (name_end < tokens.size() && tokens[name_end].Type == Lexer::Token::TokenType::OpenParen) {
            size_t depth = 0;
            size_t matching_close = 0;
            for (size_t k = name_end; k < tokens.size(); ++k) {
                if (tokens[k].Type == Lexer::Token::TokenType::OpenParen) {
                    depth++;
                } else if (tokens[k].Type == Lexer::Token::TokenType::CloseParen) {
                    depth--;
                    if (depth == 0) {
                        matching_close = k;
                        break;
                    }
                }
            }
            if (matching_close == tokens.size() - 1) {
                std::string func_name;
                for (size_t k = 0; k < name_end; ++k) {
                    func_name += tokens[k].Value;
                }
                std::vector<std::vector<Lexer::Token>> nested_args;
                std::vector<Lexer::Token> current_nested;
                size_t p_depth = 0;
                size_t b_depth = 0;
                size_t br_depth = 0;
                for (size_t k = name_end + 1; k < matching_close; ++k) {
                    const auto &tk = tokens[k];
                    if (tk.Type == Lexer::Token::TokenType::OpenParen) p_depth++;
                    else if (tk.Type == Lexer::Token::TokenType::CloseParen) p_depth--;
                    else if (tk.Type == Lexer::Token::TokenType::OpenBracket) b_depth++;
                    else if (tk.Type == Lexer::Token::TokenType::CloseBracket) b_depth--;
                    else if (tk.Type == Lexer::Token::TokenType::OpenBrace) br_depth++;
                    else if (tk.Type == Lexer::Token::TokenType::CloseBrace) br_depth--;

                    if (tk.Type == Lexer::Token::TokenType::Comma && p_depth == 0 && b_depth == 0 && br_depth == 0) {
                        nested_args.push_back(current_nested);
                        current_nested.clear();
                    } else {
                        current_nested.push_back(tk);
                    }
                }
                if (!current_nested.empty()) {
                    nested_args.push_back(current_nested);
                }
                std::vector<AST::ASTTypeRef> nested_types;
                for (const auto &na : nested_args) {
                    nested_types.push_back(InferExpressionType(na, function, module_path, name_space));
                }
                const auto *func_symbol = mAnalyser.ResolveFunction(module_path, func_name, nested_types);
                if (!func_symbol) {
                    std::string scoped_func = func_name.find("::") == std::string::npos && !name_space.empty()
                                                 ? name_space + "::" + func_name
                                                 : func_name;
                    func_symbol = mAnalyser.ResolveSymbol(module_path, func_name);
                    if (!func_symbol && scoped_func != func_name)
                        func_symbol = mAnalyser.ResolveSymbol(module_path, scoped_func);
                }
                if (func_symbol && func_symbol->Kind == AST::ASTDeclaration::Kind::Function) {
                    return func_symbol->Signature.ReturnType;
                }
            }
        }

        // 3. Check for member access
        size_t last_dot_idx = std::string::npos;
        size_t p_depth = 0;
        size_t b_depth = 0;
        size_t br_depth = 0;
        for (size_t k = 0; k < tokens.size(); ++k) {
            const auto &tk = tokens[k];
            if (tk.Type == Lexer::Token::TokenType::OpenParen) p_depth++;
            else if (tk.Type == Lexer::Token::TokenType::CloseParen) p_depth--;
            else if (tk.Type == Lexer::Token::TokenType::OpenBracket) b_depth++;
            else if (tk.Type == Lexer::Token::TokenType::CloseBracket) b_depth--;
            else if (tk.Type == Lexer::Token::TokenType::OpenBrace) br_depth++;
            else if (tk.Type == Lexer::Token::TokenType::CloseBrace) br_depth--;

            if (tk.Value == "." && p_depth == 0 && b_depth == 0 && br_depth == 0) {
                last_dot_idx = k;
            }
        }
        if (last_dot_idx != std::string::npos && last_dot_idx + 1 < tokens.size()) {
            std::vector<Lexer::Token> left_tokens(tokens.begin(), tokens.begin() + last_dot_idx);
            std::string member_name = tokens[last_dot_idx + 1].Value;
            AST::ASTTypeRef left_type = InferExpressionType(left_tokens, function, module_path, name_space);
            if (!left_type.Name.empty()) {
                return mAnalyser.GetMemberType(left_type.Name, member_name, module_path);
            }
        }

        // 4. Fallback scanner
        bool has_string = false;
        bool has_float = false;
        bool has_bool = false;
        bool has_null = false;
        AST::ASTTypeRef other_type;

        for (const auto &tk : tokens) {
            if (tk.Type == Lexer::Token::TokenType::String) {
                has_string = true;
            } else if (tk.Type == Lexer::Token::TokenType::Number) {
                if (tk.Value.find('.') != std::string::npos || tk.Value.back() == 'f' || tk.Value.back() == 'F') {
                    has_float = true;
                }
            } else if (tk.Type == Lexer::Token::TokenType::Identifier) {
                if (tk.Value == "true" || tk.Value == "false") {
                    has_bool = true;
                } else if (tk.Value == "null") {
                    has_null = true;
                } else {
                    if (function) {
                        bool found_var = false;
                        for (const auto &param : function->Parameters) {
                            if (param.Name == tk.Value) {
                                other_type = param.Type;
                                found_var = true;
                                break;
                            }
                        }
                        if (!found_var) {
                            for (const auto &local : function->LocalVariables) {
                                if (local.Name == tk.Value) {
                                    other_type = local.Type;
                                    found_var = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (other_type.Name.empty()) {
                        std::string scoped = tk.Value.find("::") == std::string::npos && !name_space.empty()
                                                 ? name_space + "::" + tk.Value
                                                 : tk.Value;
                        const auto *symbol = mAnalyser.ResolveSymbol(module_path, tk.Value);
                        if (!symbol && scoped != tk.Value)
                            symbol = mAnalyser.ResolveSymbol(module_path, scoped);

                        if (symbol && symbol->Kind == AST::ASTDeclaration::Kind::Global) {
                            other_type = mAnalyser.GetGlobalType(symbol);
                        }
                    }
                }
            }
        }

        AST::ASTTypeRef type;
        if (has_string) {
            type.Name = "string";
        } else if (has_float || (!other_type.Name.empty() && other_type.Name == "float")) {
            type.Name = "float";
        } else if (has_bool || (!other_type.Name.empty() && other_type.Name == "bool")) {
            type.Name = "bool";
        } else if (!other_type.Name.empty()) {
            return other_type;
        } else if (has_null) {
            type.Name = "null";
        } else {
            type.Name = "int";
        }
        return type;
    }

    std::string Generator::EmitDeclaration(const AST::ASTDeclaration &declaration, const std::string &module_path,
                                           const std::string &name_space) const {
        if (declaration.Type == AST::ASTDeclaration::Kind::Namespace) {
            std::string result;
            const auto &ns = *std::get<std::shared_ptr<AST::ASTNamespace>>(declaration.Data);
            const std::string nested = name_space.empty() ? ns.Name : name_space + "::" + ns.Name;
            for (const auto &child : ns.Declarations)
                result += EmitDeclaration(child, module_path, nested);
            return result;
        }
        const std::string qualified = name_space.empty() ? declaration.Name : name_space + "::" + declaration.Name;
        const auto *symbol = mAnalyser.FindDeclarationSymbol(
            qualified,
            module_path,
            declaration);
        if (!symbol)
            return {};
        if (declaration.Type == AST::ASTDeclaration::Kind::Function) {
            const auto &function = std::get<AST::ASTFunction>(declaration.Data);
            std::string result =
                EmitTypeRef(function.ReturnType, module_path, name_space) + " " + symbol->InternalName + "(";
            for (size_t i = 0; i < function.Parameters.size(); ++i) {
                if (i)
                    result += ", ";
                result += EmitTypeRef(function.Parameters[i].Type, module_path, name_space);
                if (!function.Parameters[i].Direction.empty())
                    result += function.Parameters[i].Direction;
                result += " " + function.Parameters[i].Name;
            }
            return result + ")" + JoinTokens(RewriteBody(function.Body, &function, module_path, name_space)) + "\n";
        }
        if (declaration.Type == AST::ASTDeclaration::Kind::Global) {
            const auto &global = std::get<AST::ASTGlobal>(declaration.Data);
            std::string result = EmitTypeRef(global.Type, module_path, name_space) + " " + symbol->InternalName;
            if (!global.Initializer.empty())
                result += " = " + JoinTokens(RewriteBody(global.Initializer, nullptr, module_path, name_space));
            return result + ";\n";
        }
        const auto &type = std::get<AST::ASTType>(declaration.Data);
        return "class " + symbol->InternalName + " {" +
               JoinTokens(RewriteBody(type.Body, nullptr, module_path, name_space)) + "};\n";
    }

    std::string
    Generator::GenerateMonoScript(const std::vector<std::string> &emission_order,
                                  const std::unordered_map<std::string, AST::ASTModule> &parsed_modules) const {
        std::string result;
        for (const auto &module_path : emission_order) {
            auto module = parsed_modules.find(module_path);
            if (module == parsed_modules.end())
                continue;
            for (const auto &declaration : module->second.Declarations) {
                result += EmitDeclaration(declaration, module_path, "");
            }
        }
        return result;
    }
} // namespace CE::Scripting::Impl::Codegen
