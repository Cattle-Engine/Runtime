#include "engine/scripting/private/parser.hpp"

#include <memory>
#include <unordered_set>

#include "engine/scripting/private/exceptions.hpp"
#include "engine/scripting/private/lexer.hpp"

namespace CE::Scripting::Impl::Parser {
    class Parser {
      public:
        Parser(const std::vector<Lexer::Token>& tokens) : mTokens(tokens) {}

        AST::ASTModule Parse() {
            AST::ASTModule script_module;

            while (Current().Type != Lexer::Token::TokenType::EndOfFile) {
                if (Current().Type == Lexer::Token::TokenType::KeywordImport ||
                    Current().Type == Lexer::Token::TokenType::KeywordUsing) {
                    script_module.Imports.push_back(ParseImport());
                    continue;
                }

                if (Current().Type == Lexer::Token::TokenType::KeywordExport &&
                    (Peek().Type == Lexer::Token::TokenType::KeywordImport ||
                     Peek().Type == Lexer::Token::TokenType::KeywordUsing)) {
                    Advance(); // consume 'export'
                    AST::ASTImport import = ParseImport();
                    import.Exported = true;
                    script_module.Imports.push_back(import);
                    continue;
                }

                script_module.Declarations.push_back(ParseDeclaration());
            }

            return script_module;
        }

      private:
        const Lexer::Token& Current() {
            return mTokens.at(mPosition);
        }
        void Advance() {
            mPosition++;
        }

        bool Match(Lexer::Token::TokenType type) {
            if (Current().Type != type) {
                return false;
            }

            mPosition++;
            return true;
        }

        std::string TokenTypeName(Lexer::Token::TokenType type) {
            using TokenType = Lexer::Token::TokenType;

            switch (type) {
                case TokenType::KeywordImport:
                    return "'import'";
                case TokenType::KeywordExport:
                    return "'export'";
                case TokenType::KeywordNamespace:
                    return "'namespace'";
                case TokenType::KeywordClass:
                    return "'class'";
                case TokenType::KeywordStruct:
                    return "'struct'";
                case TokenType::KeywordConst:
                    return "'const'";
                case TokenType::KeywordAuto:
                    return "'auto'";
                case TokenType::KeywordUsing:
                    return "'using'";
                case TokenType::OpenBracket:
                    return "'['";
                case TokenType::CloseBracket:
                    return "']'";
                case TokenType::Handle:
                    return "'@'";
                case TokenType::Reference:
                    return "'&'";
                case TokenType::ScopeResolution:
                    return "'::'";
                case TokenType::Assignment:
                    return "'='";
                case TokenType::Identifier:
                    return "an identifier";
                case TokenType::String:
                    return "a string literal";
                case TokenType::Number:
                    return "a number literal";
                case TokenType::Symbol:
                    return "a symbol";
                case TokenType::OpenBrace:
                    return "'{'";
                case TokenType::CloseBrace:
                    return "'}'";
                case TokenType::OpenParen:
                    return "'('";
                case TokenType::CloseParen:
                    return "')'";
                case TokenType::Comma:
                    return "','";
                case TokenType::Semicolon:
                    return "';'";
                case TokenType::EndOfFile:
                    return "end of file";
                default:
                    return "another token";
            }
        }

        std::string TokenDescription(const Lexer::Token& token) {
            if (token.Type == Lexer::Token::TokenType::EndOfFile) {
                return "end of file";
            }

            if (!token.Value.empty()) {
                return "'" + token.Value + "'";
            }

            return TokenTypeName(token.Type);
        }

        bool IsAngelScriptGrammarKeyword(const std::string& value) {
            static const std::unordered_set<std::string> keywords = {
                "enum",
                "typedef",
                "funcdef",
                "property",
                "interface",
                "mixin",
                "shared",
                "override",
                "final",
                "private",
                "protected",
                "public",
                "abstract",
                "virtual",
                "const",
                "in",
                "out",
                "inout",
                "return",
                "break",
                "continue",
                "if",
                "else",
                "for",
                "while",
                "switch",
                "case",
                "default",
                "do",
                "try",
                "catch",
                "throws",
                "class",
                "struct",
                "namespace",
                "enum",
                "import",
                "using",
                "export"};
            return keywords.contains(value);
        }

        AST::ASTRawDeclaration ParseRawDeclaration(const std::string& kind = "declaration") {
            AST::ASTRawDeclaration raw;
            raw.Kind = kind;

            while (Current().Type != Lexer::Token::TokenType::EndOfFile) {
                raw.Tokens.push_back(Current());
                if (Current().Type == Lexer::Token::TokenType::Semicolon) {
                    Advance();
                    break;
                }

                if (Current().Type == Lexer::Token::TokenType::OpenBrace) {
                    int brace_depth = 1;
                    Advance();
                    while (brace_depth > 0 && Current().Type != Lexer::Token::TokenType::EndOfFile) {
                        raw.Tokens.push_back(Current());
                        if (Current().Type == Lexer::Token::TokenType::OpenBrace) {
                            brace_depth++;
                        } else if (Current().Type == Lexer::Token::TokenType::CloseBrace) {
                            brace_depth--;
                        }
                        Advance();
                    }

                    if (Current().Type == Lexer::Token::TokenType::Semicolon) {
                        raw.Tokens.push_back(Current());
                        Advance();
                    }
                    break;
                }

                Advance();
            }

            if (Current().Type == Lexer::Token::TokenType::EndOfFile && raw.Tokens.empty()) {
                Error("expected a declaration, but reached end of file");
            }

            return raw;
        }

        const Lexer::Token& Expect(
            Lexer::Token::TokenType type,
            const std::string& context = {}
        ) {
            if (Current().Type != type) {
                std::string message = "expected " + TokenTypeName(type);

                if (!context.empty()) {
                    message += " " + context;
                }

                message += ", but found " + TokenDescription(Current());

                Error(message);
            }

            return mTokens[mPosition++];
        }

        [[noreturn]] void Error(
            const std::string& message,
            const Lexer::Token& token
        ) {
            throw Exceptions::ParserError(message, token.Location);
        }

        [[noreturn]] void Error(const std::string& message) {
            Error(message, Current());
        }

        const Lexer::Token& Peek(size_t offset = 1) {
            size_t idx = mPosition + offset;
            if (idx >= mTokens.size()) {
                return mTokens.back();
            }
            return mTokens[idx];
        }

        bool IsTypeRefAhead() {
            // A qualified type is detected at its first component only.
            // Without this guard `MyType::Nested value` is recorded twice,
            // once as MyType::Nested and again at Nested.
            if (mPosition > 0 &&
                mTokens[mPosition - 1].Type ==
                    Lexer::Token::TokenType::ScopeResolution) {
                return false;
            }
            if (Current().Type == Lexer::Token::TokenType::KeywordConst ||
                Current().Type == Lexer::Token::TokenType::KeywordAuto) {
                return true;
            }
            if (Current().Type != Lexer::Token::TokenType::Identifier) {
                return false;
            }

            size_t scanPos = mPosition + 1; // first identifier already consumed
            while (scanPos + 1 < mTokens.size() && mTokens[scanPos].Type == Lexer::Token::TokenType::ScopeResolution &&
                   mTokens[scanPos + 1].Type == Lexer::Token::TokenType::Identifier) {
                scanPos += 2;
            }

            if (scanPos < mTokens.size()) {
                auto nextT = mTokens[scanPos].Type;
                if (nextT == Lexer::Token::TokenType::Handle || nextT == Lexer::Token::TokenType::Reference ||
                    nextT == Lexer::Token::TokenType::Identifier) {
                    return true;
                }
            }
            return false;
        }

        AST::ASTImport ParseImport() {
            AST::ASTImport result;
            result.Location = Current().Location;

            // consume either 'import' or 'using'
            if (Current().Type != Lexer::Token::TokenType::KeywordImport &&
                Current().Type != Lexer::Token::TokenType::KeywordUsing) {
                throw Exceptions::ParserError(
                    "Expected 'import' or 'using'",
                    Current().Location
                );
            }

            // store whether this is 'using' vs 'import'
            result.IsUsing =
                (Current().Type == Lexer::Token::TokenType::KeywordUsing);
            Advance();

            // parse the first identifier
            if (Current().Type != Lexer::Token::TokenType::Identifier) {
                throw Exceptions::ParserError(
                    "Expected identifier",
                    Current().Location
                );
            }

            std::string first_part = Current().Value;
            Advance();

            // check if we have a scope resolution (::)
            if (Current().Type == Lexer::Token::TokenType::ScopeResolution) {
                Advance(); // consume ::

                // we have module::symbol
                if (Current().Type != Lexer::Token::TokenType::Identifier) {
                    throw Exceptions::ParserError(
                        "Expected symbol name after '::'",
                        Current().Location
                    );
                }

                result.Module = first_part;
                result.Symbol = Current().Value;
                result.Path.push_back(first_part); // keep path for compatibility
                result.Path.push_back(Current().Value);
                Advance();
            } else {
                // we just have a module/file import
                result.Module = first_part;
                result.Path.push_back(first_part);
                // result.Symbol remains nullopt)
            }

            Expect(Lexer::Token::TokenType::Semicolon);
            return result;
        }

        AST::ASTFunction ParseFunction(
            AST::ASTTypeRef type,
            std::string name
        ) {
            AST::ASTFunction func;
            func.Name = name;
            func.ReturnType = type;

            Expect(
                Lexer::Token::TokenType::OpenParen,
                "after function name '" + name + "'"
            );

            while (Current().Type != Lexer::Token::TokenType::CloseParen) {
                if (Current().Type ==
                    Lexer::Token::TokenType::EndOfFile) {
                    Error(
                        "expected ')' to close parameter list of function '" +
                        name + "', but reached end of file"
                    );
                }
                AST::ASTParameter param;
                param.Type = ParseTypeRef();

                if (Current().Type ==
                        Lexer::Token::TokenType::Identifier &&
                    (Current().Value == "in" ||
                     Current().Value == "out" ||
                     Current().Value == "inout")) {
                    param.Direction = Current().Value;
                    Advance();
                }

                param.Name = Expect(
                    Lexer::Token::TokenType::Identifier,
                    "as the name of a parameter in function '" + name + "'"
                ).Value;
                func.Parameters.push_back(param);

                if (Current().Type !=
                    Lexer::Token::TokenType::CloseParen) {
                    Expect(
                        Lexer::Token::TokenType::Comma,
                        "between parameters of function '" + name + "'"
                    );
                }
            }

            Expect(
                Lexer::Token::TokenType::CloseParen,
                "to close parameter list of function '" + name + "'"
            );

            size_t body_start_idx = mPosition;
            Expect(
                Lexer::Token::TokenType::OpenBrace,
                "to begin body of function '" + name + "'"
            );

            int brace_depth = 1;
            while (brace_depth > 0 &&
                   Current().Type !=
                       Lexer::Token::TokenType::EndOfFile) {
                if (IsTypeRefAhead()) {
                    size_t saved_pos = mPosition;

                    AST::ASTLocalVariable local;
                    local.Type = ParseTypeRef();
                    local.Name = Expect(
                        Lexer::Token::TokenType::Identifier,
                        "as the name of a local variable in function '" +
                            name + "'"
                    ).Value;

                    func.LocalVariables.push_back(local);

                    mPosition = saved_pos;
                }

                if (Current().Type ==
                    Lexer::Token::TokenType::OpenBrace) {
                    brace_depth++;
                } else if (Current().Type ==
                           Lexer::Token::TokenType::CloseBrace) {
                    brace_depth--;
                }

                Advance();
            }

            if (brace_depth != 0) {
                Error(
                    "expected '}' to close body of function '" +
                    name + "', but reached end of file"
                );
            }

            func.Body.assign(
                mTokens.begin() + static_cast<std::ptrdiff_t>(body_start_idx),
                mTokens.begin() + static_cast<std::ptrdiff_t>(mPosition)
            );

            return func;
        }

        AST::ASTGlobal ParseGlobal(
            AST::ASTTypeRef type,
            std::string name
        ) {
            AST::ASTGlobal global;

            global.Type = type;
            global.Name = name;

            if (Current().Type == Lexer::Token::TokenType::Assignment) {
                Advance();

                std::vector<Lexer::Token> initializer;
                int paren_count = 0;

                while (Current().Type !=
                       Lexer::Token::TokenType::Semicolon) {
                    if (Current().Type ==
                        Lexer::Token::TokenType::EndOfFile) {
                        Error(
                            "expected ';' after initializer of global '" +
                            name + "', but reached end of file"
                        );
                    }

                    if (Current().Type ==
                        Lexer::Token::TokenType::OpenParen) {
                        paren_count++;
                    } else if (Current().Type ==
                               Lexer::Token::TokenType::CloseParen) {
                        if (paren_count == 0) {
                            Error(
                                "unexpected ')' in initializer of global '" +
                                name + "'"
                            );
                        }
                        paren_count--;
                    }

                    initializer.push_back(Current());
                    Advance();
                }

                if (paren_count > 0) {
                    Error(
                        "expected ')' in initializer of global '" +
                        name + "', but reached ';'"
                    );
                }
                global.Initializer = std::move(initializer);
            }

            Expect(
                Lexer::Token::TokenType::Semicolon,
                "after global declaration '" + name + "'"
            );
            return global;
        }

        AST::ASTType ParseType() {
            AST::ASTType type;

            const bool is_class =
                Current().Type == Lexer::Token::TokenType::KeywordClass;
            const std::string type_keyword =
                is_class ? "class" : "struct";
            // skip class/struct keyword
            Advance();

            type.Name = Expect(
                Lexer::Token::TokenType::Identifier,
                "as the name of " + type_keyword
            ).Value;
            Expect(
                Lexer::Token::TokenType::OpenBrace,
                "to begin " + type_keyword + " '" + type.Name + "'"
            );

            // parse the body
            std::vector<Lexer::Token> body;
            int brace_depth = 1;

            while (brace_depth > 0 &&
                   Current().Type !=
                       Lexer::Token::TokenType::EndOfFile) {
                if (Current().Type ==
                    Lexer::Token::TokenType::OpenBrace) {
                    brace_depth++;
                }

                if (Current().Type ==
                    Lexer::Token::TokenType::CloseBrace) {
                    brace_depth--;
                }

                if (brace_depth > 0) {
                    body.push_back(Current());
                }

                Advance();
            }

            if (Current().Type ==
                Lexer::Token::TokenType::EndOfFile) {
                Error(
                    "expected '}' to close " + type_keyword +
                    " '" + type.Name + "', but reached end of file"
                );
            }
            type.Body = std::move(body);

            Expect(
                Lexer::Token::TokenType::Semicolon,
                "after " + type_keyword + " '" + type.Name + "'"
            );
            return type;
        }

        AST::ASTTypeRef ParseTypeRef() {
            AST::ASTTypeRef typeref;

            // check for const
            if (Current().Type ==
                Lexer::Token::TokenType::KeywordConst) {
                typeref.IsConst = true;
                Advance();
            }

            if (Current().Type ==
                Lexer::Token::TokenType::KeywordAuto) {
                typeref.IsAuto = true;
                Advance();
                return typeref; // cant have modifiers with auto
            }

            typeref.Name = Expect(
                Lexer::Token::TokenType::Identifier,
                "as a type name"
            ).Value;

            while (Current().Type ==
                   Lexer::Token::TokenType::ScopeResolution) {
                Advance(); // consume the ::

                typeref.Name += "::" + Expect(
                    Lexer::Token::TokenType::Identifier,
                    "as a type name after '" +
                        typeref.Name + "::'"
                ).Value;
            }

            // parse modifiers
            while (true) {
                if (Current().Type ==
                    Lexer::Token::TokenType::Handle) {
                    typeref.IsHandle = true;
                    Advance();
                } else if (Current().Type ==
                           Lexer::Token::TokenType::Reference) {
                    typeref.IsReference = true;
                    Advance();
                } else if (Current().Type ==
                           Lexer::Token::TokenType::OpenBracket) {
                    typeref.ArrayDepth++;
                    Advance();

                    Expect(
                        Lexer::Token::TokenType::CloseBracket,
                        "to close array type"
                    );
                } else {
                    break;
                }
            }

            return typeref;
        }

        AST::ASTNamespace ParseNamespace() {
            AST::ASTNamespace ns;

            Expect(
                Lexer::Token::TokenType::KeywordNamespace,
                "to begin a namespace declaration"
            );

            ns.Name = Expect(
                Lexer::Token::TokenType::Identifier,
                "as the namespace name"
            ).Value;

            Expect(
                Lexer::Token::TokenType::OpenBrace,
                "after namespace '" + ns.Name + "'"
            );

            while (Current().Type !=
                       Lexer::Token::TokenType::CloseBrace &&
                   Current().Type !=
                       Lexer::Token::TokenType::EndOfFile) {
                AST::ASTDeclaration decl = ParseDeclaration();

                decl.NameSpace =
                    decl.NameSpace.empty()
                        ? ns.Name
                        : ns.Name + "::" + decl.NameSpace;

                ns.Declarations.push_back(std::move(decl));
            }

            Expect(
                Lexer::Token::TokenType::CloseBrace,
                "to close namespace '" + ns.Name + "'"
            );
            return ns;
        }

        AST::ASTDeclaration ParseDeclaration() {
            AST::ASTDeclaration decl;

            decl.Exported =
                Match(Lexer::Token::TokenType::KeywordExport);
            decl.Location = Current().Location;

            if (Current().Type ==
                Lexer::Token::TokenType::KeywordNamespace) {
                auto ns =
                    std::make_shared<AST::ASTNamespace>(ParseNamespace());

                decl.Type = AST::ASTDeclaration::Kind::Namespace;
                decl.Name = ns->Name;
                decl.Data = ns;

                return decl;
            }

            if (Current().Type ==
                    Lexer::Token::TokenType::KeywordClass ||
                Current().Type ==
                    Lexer::Token::TokenType::KeywordStruct) {
                auto type = ParseType();

                decl.Type = AST::ASTDeclaration::Kind::Type;
                decl.Name = type.Name;
                decl.Data = std::move(type);

                return decl;
            }

            if (Current().Type ==
                Lexer::Token::TokenType::KeywordImport ||
                Current().Type ==
                    Lexer::Token::TokenType::KeywordUsing) {
                Error("module imports must be at module scope; 'import' and 'using' are not declarations inside a namespace or type");
            }

            if (Current().Type ==
                Lexer::Token::TokenType::EndOfFile) {
                Error("expected declaration, but reached end of file");
            }

            if (Current().Type == Lexer::Token::TokenType::Identifier) {
                const std::string& value = Current().Value;
                if (IsAngelScriptGrammarKeyword(value) && value != "class" && value != "struct" && value != "namespace") {
                    auto raw = ParseRawDeclaration(value);
                    decl.Type = AST::ASTDeclaration::Kind::Raw;
                    decl.Name = value;
                    decl.Data = std::move(raw);
                    return decl;
                }
            }

            if (Current().Type == Lexer::Token::TokenType::Handle ||
                Current().Type == Lexer::Token::TokenType::OpenBrace ||
                Current().Type == Lexer::Token::TokenType::CloseBrace ||
                Current().Type == Lexer::Token::TokenType::Semicolon) {
                auto raw = ParseRawDeclaration("attribute");
                decl.Type = AST::ASTDeclaration::Kind::Raw;
                decl.Name = "attribute";
                decl.Data = std::move(raw);
                return decl;
            }

            AST::ASTTypeRef type = ParseTypeRef();

            std::string name = Expect(
                Lexer::Token::TokenType::Identifier,
                "as the declaration name"
            ).Value;

            if (Current().Type ==
                Lexer::Token::TokenType::OpenParen) {
                auto fn = ParseFunction(type, name);

                decl.Type = AST::ASTDeclaration::Kind::Function;
                decl.Name = name;
                decl.Data = std::move(fn);

                return decl;
            }

            auto global = ParseGlobal(type, name);

            decl.Type = AST::ASTDeclaration::Kind::Global;
            decl.Name = name;
            decl.Data = std::move(global);

            return decl;
        }

        const std::vector<Lexer::Token>& mTokens;
        size_t mPosition = 0;
    };

    AST::ASTModule ParseLexerOutput(std::vector<Lexer::Token> tokens) {
        Parser parser(tokens);
        return parser.Parse();
    }
} // namespace CE::Scripting::Impl::Parser