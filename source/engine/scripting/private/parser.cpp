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
                
                bool IsTypeRefAhead() {
                    if (Current().Type == Lexer::Token::TokenType::KeywordConst ||
                        Current().Type == Lexer::Token::TokenType::KeywordAuto) {
                        return true;
                    }
                    
                    if (Current().Type == Lexer::Token::TokenType::Identifier) {
                        size_t scanPos = mPosition;
                        while (scanPos < mTokens.size() && 
                               (mTokens[scanPos].Type == Lexer::Token::TokenType::Identifier ||
                                mTokens[scanPos].Type == Lexer::Token::TokenType::ScopeResolution)) {
                            scanPos++;
                        }
                        
                        if (scanPos < mTokens.size()) {
                            auto nextT = mTokens[scanPos].Type;
                            if (nextT == Lexer::Token::TokenType::Handle || 
                                nextT == Lexer::Token::TokenType::Reference ||
                                nextT == Lexer::Token::TokenType::Identifier) {
                                return true;
                            }
                        }
                    }
                    return false;
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
                    AST::ASTFunction func;
                    func.Name = name;
                    func.ReturnType = type;
                    
                    Expect(Lexer::Token::TokenType::OpenParen);
                    
                    while (Current().Type != Lexer::Token::TokenType::CloseParen) {
                        AST::ASTParameter param;
                        param.Type = ParseTypeRef();
                        param.Name = Expect(Lexer::Token::TokenType::Identifier).Value;
                        func.Parameters.push_back(param);
                        
                        if (Current().Type != Lexer::Token::TokenType::CloseParen) {
                            Expect(Lexer::Token::TokenType::Comma);
                        }
                    }
                    
                    Expect(Lexer::Token::TokenType::CloseParen);
                    
                    size_t body_start_idx = mPosition; 
                    Expect(Lexer::Token::TokenType::OpenBrace);
                    
                    int brace_depth = 1;
                    while (brace_depth > 0 && Current().Type != Lexer::Token::TokenType::EndOfFile) {
                        if (IsTypeRefAhead()) {
                            size_t saved_pos = mPosition;
                            
                            AST::ASTLocalVariable local;
                            local.Type = ParseTypeRef();
                            local.Name = Expect(Lexer::Token::TokenType::Identifier).Value;
                            func.LocalVariables.push_back(local);
                            
                            mPosition = saved_pos; 
                        }
                        
                        if (Current().Type == Lexer::Token::TokenType::OpenBrace) brace_depth++;
                        if (Current().Type == Lexer::Token::TokenType::CloseBrace) brace_depth--;
                        
                        Advance();
                    }

                    std::string body;
                    for (size_t i = body_start_idx; i < mPosition; ++i) {
                        body += mTokens[i].Value + " ";
                    }
                    
                    func.Body = body;
                    return func;
                }

                AST::ASTGlobal ParseGlobal(AST::ASTTypeRef type, std::string name) {
                    AST::ASTGlobal global;

                    global.Type = type;
                    global.Name = name;

                    // check for an initializer
                    if (Current().Type == Lexer::Token::TokenType::Assignment) {
                        Advance(); // consume the = 

                        std::string initializer;
                        int paren_count = 0;

                        // parse the initializer until ';'
                        while (Current().Type != Lexer::Token::TokenType::Semicolon || 
                                paren_count > 0) {
                                if (Current().Type == Lexer::Token::TokenType::OpenParen) paren_count++;
                                if (Current().Type == Lexer::Token::TokenType::CloseParen) paren_count--;
                                
                                initializer += Current().Value + " ";
                                Advance();
                            }
                            
                            global.Initializer = initializer;
                        }

                        Expect(Lexer::Token::TokenType::Semicolon);
                        return global;
                }

                AST::ASTType ParseType() {
                    AST::ASTType type;

                    // skip class/struct keyword
                    Advance();

                    type.Name = Expect(Lexer::Token::TokenType::Identifier).Value;
                    Expect(Lexer::Token::TokenType::OpenBrace);

                    // parse the body
                    std::string body;
                    int brace_depth = 1;

                    while (brace_depth > 0) {
                        if (Current().Type == Lexer::Token::TokenType::OpenBrace) brace_depth++;
                        if (Current().Type == Lexer::Token::TokenType::CloseBrace) brace_depth--;

                        if (brace_depth > 0) {
                            body += Current().Value + " ";
                        }

                        Advance();
                    }

                    type.Body = body;
                    return type;
                }

                AST::ASTTypeRef ParseTypeRef() {
                    AST::ASTTypeRef typeref;
                    
                    // check for const
                    if (Current().Type == Lexer::Token::TokenType::KeywordConst) {
                        typeref.IsConst = true;
                        Advance();
                    }

                    if (Current().Type == Lexer::Token::TokenType::KeywordAuto) {
                        typeref.IsAuto = true;
                        Advance();
                        return typeref; // cant have modifiers with auto
                    }

                    typeref.Name = Expect(Lexer::Token::TokenType::Identifier).Value;
                    while (Current().Type == Lexer::Token::TokenType::ScopeResolution) {
                        Advance(); // consume the ::
                        typeref.Name += "::" + Expect(Lexer::Token::TokenType::Identifier).Value;
                    }

                    // parse modifiers
                    while (true) {
                        if (Current().Type == Lexer::Token::TokenType::Handle) {
                            typeref.IsHandle = true;
                            Advance();
                        } else if (Current().Type == Lexer::Token::TokenType::Reference) {
                            typeref.IsReference = true;
                            Advance();
                        } else if (Current().Type == Lexer::Token::TokenType::OpenBracket) {
                            typeref.ArrayDepth++;
                            Advance();
                            Expect(Lexer::Token::TokenType::CloseBracket);
                        } else {
                            break;
                        }
                    }

                    return typeref;
                }

                AST::ASTNamespace ParseNamespace() {
                    AST::ASTNamespace ns;
                    
                    Expect(Lexer::Token::TokenType::KeywordNamespace);
                    ns.Name = Expect(Lexer::Token::TokenType::Identifier).Value;
                    Expect(Lexer::Token::TokenType::OpenBrace);
                    
                    while (Current().Type != Lexer::Token::TokenType::CloseBrace && 
                        Current().Type != Lexer::Token::TokenType::EndOfFile) {
                        ns.Declarations.push_back(ParseDeclaration());
                    }
                    
                    Expect(Lexer::Token::TokenType::CloseBrace);
                    
                    return ns;
                }

                AST::ASTDeclaration ParseDeclaration() {
                    AST::ASTDeclaration decl;

                    decl.Exported = Match(Lexer::Token::TokenType::KeywordExport);

                    if (Current().Type == Lexer::Token::TokenType::KeywordNamespace) {
                        auto ns = std::make_shared<AST::ASTNamespace>(ParseNamespace());

                        decl.Type = AST::ASTDeclaration::Kind::Namespace;
                        decl.Name = ns->Name;
                        decl.Data = ns;

                        return decl;
                    }

                    if (Current().Type == Lexer::Token::TokenType::KeywordClass ||
                        Current().Type == Lexer::Token::TokenType::KeywordStruct) {

                        auto type = ParseType();

                        decl.Type = AST::ASTDeclaration::Kind::Type;
                        decl.Name = type.Name;
                        decl.Data = std::move(type);

                        return decl;
                    }

                    AST::ASTTypeRef type = ParseTypeRef();
                    std::string name = Expect(Lexer::Token::TokenType::Identifier).Value;

                    if (Current().Type == Lexer::Token::TokenType::OpenParen) {
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
}