#include <memory>

#include "engine/scripting/private/parser.hpp"
#include "engine/scripting/private/exceptions.hpp"

namespace CE::Scripting::Impl::Parser {
    class Parser {
        public:
                Parser(const std::vector<Lexer::Token>& tokens) : mTokens(tokens) {}

                AST::ASTModule Parse() {
                    AST::ASTModule script_module;

                    while (Current().Type != Lexer::Token::TokenType::EndOfFile) {
                        if (Current().Type == Lexer::Token::TokenType::KeywordImport) {
                            script_module.Imports.push_back(ParseImport());
                            continue;
                        } else {
                            script_module.Declarations.push_back(ParseDeclaration());
                        }
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

                const Lexer::Token& Expect(Lexer::Token::TokenType type) {
                    if (Current().Type != type) {
                        throw Exceptions::ParserError("Unexpected token", Current().Location);
                    }

                    return mTokens[mPosition++];
                }

                AST::ASTImport ParseImport() {
                    AST::ASTImport result;

                    result.Location = Current().Location;

                    Expect(Lexer::Token::TokenType::KeywordImport);

                    std::vector<std::string> parts;

                    while (Current().Type == Lexer::Token::TokenType::Identifier) {
                        parts.push_back(Current().Value);
                        Advance();

                        if (Current().Value != "::") {
                            break;
                        }

                        Advance();
                    }

                    Expect(Lexer::Token::TokenType::Semicolon);

                    if (parts.empty()) {
                        throw Exceptions::ParserError("Expected import path", Current().Location);
                    }

                    if (parts.size() == 1) {
                        result.Module = parts[0];
                    } else {
                        for (size_t i = 0; i < parts.size() - 1; i++) {
                            if (!result.Module.empty()) {
                                result.Module += "::";
                            }

                            result.Module += parts[i];
                        }

                        result.Symbol = parts.back();
                    }

                    return result;
                }

                AST::ASTFunction ParseFunction(AST::ASTTypeRef type, std::string name) {

                }


                AST::ASTFunction ParseGlobal(AST::ASTTypeRef type, std::string name) {

                }

                AST::ASTType ParseType() {

                }

                AST::ASTTypeRef ParseTypeRef() {

                }

                AST::ASTNamespace ParseNamespace() {

                }

                AST::ASTDeclaration ParseDeclaration() {
                    AST::ASTDeclaration decl;

                    decl.Exported = Match(Lexer::Token::TokenType::KeywordExport);

                    if (Current().Type == Lexer::Token::TokenType::KeywordNamespace) {
                        auto ns = std::make_shared<AST::ASTNamespace>(ParseNamespace());

                        decl.Kind = AST::ASTDeclaration::Kind::Namespace;
                        decl.Name = ns->Name;
                        decl.Data = ns;

                        return decl;
                    }

                    if (Current().Type == Lexer::Token::TokenType::KeywordClass ||
                        Current().Type == Lexer::Token::TokenType::KeywordStruct) {

                        auto type = ParseType();

                        decl.Kind = AST::ASTDeclaration::Kind::Type;
                        decl.Name = type.Name;
                        decl.Data = std::move(type);

                        return decl;
                    }

                    AST::ASTTypeRef type = ParseTypeRef();
                    std::string name = Expect(Lexer::Token::TokenType::Identifier).Value;

                    if (Current().Type == Lexer::Token::TokenType::OpenParen) {
                        auto fn = ParseFunction(type, name);

                        decl.Kind = AST::ASTDeclaration::Kind::Function;
                        decl.Name = name;
                        decl.Data = std::move(fn);

                        return decl;
                    }

                    auto global = ParseGlobal(type, name);

                    decl.Kind = AST::ASTDeclaration::Kind::Global;
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
}