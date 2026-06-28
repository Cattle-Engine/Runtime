#include "engine/scripting/private/lexer.hpp"

namespace CE::Scripting::Impl::Lexer {
    std::vector<Token> Lex(const std::string& data) {
        std::vector<Token> tokens;

        size_t position = 0;

        while (position < data.size()) {
            char current = data[position];

            if (std::isspace(current)) {
                position++;
                continue;
            }

            if (std::isalpha(current) || current == '_') {
                std::string value;

                while (position < data.size() &&
                       (std::isalnum(data[position]) || data[position] == '_')) {
                    value += data[position];
                    position++;
                }

                Token token;

                if (value == "import") {
                    token.Type = Token::TokenType::KeywordImport;
                } else if (value == "export") {
                    token.Type = Token::TokenType::KeywordExport;
                } else if (value == "namespace") {
                    token.Type = Token::TokenType::KeywordNamespace;
                } else {
                    token.Type = Token::TokenType::Identifier;
                }

                token.Value = value;
                tokens.push_back(token);
                continue;
            }

            // Handle numbers
            if (std::isdigit(current)) {
                std::string value;

                while (position < data.size() &&
                       std::isdigit(data[position])) {
                    value += data[position];
                    position++;
                }

                Token token;

                token.Type = Token::TokenType::Number;
                token.Value = value;

                tokens.push_back(token);
                continue;
            }

            if (current = '"')  {
                position++;

                std::string value;

                while (position < data.size() && data[position] != '"') {
                    value += data[position];
                    position++;
                }

                position++; // skip the closing "

                Token token;

                token.Type = Token::TokenType::String;
                token.Value = value;

                tokens.push_back(token);
                continue;
            }
        }
    }                                                                                                                                                                                                                                                                                                                                                                                        
}