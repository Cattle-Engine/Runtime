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
                        }
                    }
                }
        private:
                const Lexer::Token& Current() {
                    return mTokens.at(mPosition);
                }

                Lexer::Token& Advance() {
                    mPosition++;
                }

                bool Match(Lexer::Token::TokenType type) {

                }

                void Expect(Lexer::Token::TokenType type) {

                }

                AST::ASTImport Parser::ParseImport() {
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
                AST::ASTDeclaration ParseDeclaration() {

                }

                AST::ASTNamespace ParseNamespace() {

                }
    
                const std::vector<Lexer::Token>& mTokens;
                size_t mPosition = 0;
        };


    AST::ASTModule ParseLexerOutput(std::vector<Lexer::Token> tokens) {
        Parser parser(tokens);
        return parser.Parse();
    }
}