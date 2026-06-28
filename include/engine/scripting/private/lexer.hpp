#pragma once

#include <string>
#include <vector>

#include "engine/scripting/private/modules.hpp"

namespace CE::Scripting::Impl::Lexer {
    struct Token {
        enum class TokenType {
            KeywordImport,
            KeywordExport,
            KeywordNamespace,

            Identifier,
            String,
            Number,
            Symbol,

            OpenBrace,
            CloseBrace,
            OpenParen,
            CloseParen,

            Comma,
            Semicolon,

            EndOfFile
        };

        TokenType Type= TokenType::EndOfFile;

        std::string Value = "";

        SourceLocation Location{};
    };
    
    // Throws CE::Runtime::Impl::Exceptioms::LexerError when an errpr occurs
    std::vector<Token> Lex(const std::string& data);
}