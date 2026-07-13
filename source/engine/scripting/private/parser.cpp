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
                        }
                        
                        if (Current().Type == Lexer::Token::TokenType::KeywordExport &&
                            Peek().Type == Lexer::Token::TokenType::KeywordImport) {
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

                const Lexer::Token& Expect(Lexer::Token::TokenType type) {
                    if (Current().Type != type) {
                        throw Exceptions::ParserError("Unexpected token", Current().Location);
                    }

                    return mTokens[mPosition++];
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
                    if (mPosition > 0 && mTokens[mPosition - 1].Type == Lexer::Token::TokenType::ScopeResolution) {
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
                    while (scanPos + 1 < mTokens.size() &&
                        mTokens[scanPos].Type == Lexer::Token::TokenType::ScopeResolution &&
                        mTokens[scanPos + 1].Type == Lexer::Token::TokenType::Identifier) {
                        scanPos += 2;
                    }

                    if (scanPos < mTokens.size()) {
                        auto nextT = mTokens[scanPos].Type;
                        if (nextT == Lexer::Token::TokenType::Handle ||
                            nextT == Lexer::Token::TokenType::Reference ||
                            nextT == Lexer::Token::TokenType::Identifier) {
                            return true;
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
                        result.Path.push_back(Current().Value);
                        parts.push_back(Current().Value);
                        Advance();

                        if (Current().Type != Lexer::Token::TokenType::ScopeResolution) {
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
                        result.Path.pop_back();
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
                        if (Current().Type == Lexer::Token::TokenType::Identifier &&
                            (Current().Value == "in" || Current().Value == "out" || Current().Value == "inout")) {
                            param.Direction = Current().Value;
                            Advance();
                        }
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

                    func.Body.assign(mTokens.begin() + body_start_idx, mTokens.begin() + mPosition);
                    return func;
                }

                AST::ASTGlobal ParseGlobal(AST::ASTTypeRef type, std::string name) {
                    AST::ASTGlobal global;

                    global.Type = type;
                    global.Name = name;

                    // check for an initializer
                    if (Current().Type == Lexer::Token::TokenType::Assignment) {
                        Advance(); // consume the = 

                        std::vector<Lexer::Token> initializer;
                        int paren_count = 0;

                        // parse the initializer until ';'
                        while (Current().Type != Lexer::Token::TokenType::Semicolon || 
                                paren_count > 0) {
                                if (Current().Type == Lexer::Token::TokenType::OpenParen) paren_count++;
                                if (Current().Type == Lexer::Token::TokenType::CloseParen) paren_count--;
                                
                                initializer.push_back(Current());
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
                    std::vector<Lexer::Token> body;
                    int brace_depth = 1;

                    while (brace_depth > 0) {
                        if (Current().Type == Lexer::Token::TokenType::OpenBrace) brace_depth++;
                        if (Current().Type == Lexer::Token::TokenType::CloseBrace) brace_depth--;

                        if (brace_depth > 0) {
                            body.push_back(Current());
                        }

                        Advance();
                    }

                    type.Body = body;
                    Match(Lexer::Token::TokenType::Semicolon);
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
                        AST::ASTDeclaration decl = ParseDeclaration();

                        decl.NameSpace = decl.NameSpace.empty()
                            ? ns.Name
                            : ns.Name + "::" + decl.NameSpace;

                        ns.Declarations.push_back(std::move(decl));
                    }

                    Expect(Lexer::Token::TokenType::CloseBrace);
                    return ns;
                }

                AST::ASTDeclaration ParseDeclaration() {
                    AST::ASTDeclaration decl;

                    decl.Exported = Match(Lexer::Token::TokenType::KeywordExport);
                    decl.Location = Current().Location;

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
