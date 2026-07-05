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
            KeywordClass,
            KeywordStruct,

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

        TokenType Type = TokenType::EndOfFile;

        std::string Value = "";

        SourceLocation Location{};
    };
    
    /**
     * @brief Parses a script file as a string and returns tokens
     * @return Returns a std::vector of tokens or throws CE::Runtime::Impl::Exceptioms::LexerError when an error occurs
     * 
     * @param data The script file
     * @param filename Used for logging purposes
     */
    std::vector<Token> Lex(const std::string& data, const std::string& filename);
}