#include "engine/scripting/private/lexer.hpp"
#include "engine/scripting/private/exceptions.hpp"

namespace CE::Scripting::Impl::Lexer {
    std::vector<Token> Lex(const std::string& data, const std::string& filename) {
        std::vector<Token> tokens;

        size_t position = 0;
        uint32_t line = 1;
        uint32_t column = 1;

        auto advance = [&](size_t count = 1) {
            for (size_t i = 0; i < count && position < data.size(); ++i) {
                if (data[position] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
                position++;
            }
        };

        while (position < data.size()) {
            char current = data[position];

            // skip if a space
            if (std::isspace(current)) {
                advance();
                continue;
            }

            // Handle comments
            if (current == '/' && position + 1 < data.size()) {
                char next = data[position + 1];

                // Single-line comment
                if (next == '/') {
                    SourceLocation commentStart{ filename, line, column };
                    advance(2); // Skip '//'
                    while (position < data.size() && data[position] != '\n') {
                        advance();
                    }
                    continue;
                }

                // Multi-line comment
                if (next == '*') {
                    SourceLocation commentStart{ filename, line, column };
                    advance(2); // Skip '/*'
                    bool closed = false;

                    while (position < data.size()) {
                        if (data[position] == '*' && position + 1 < data.size() && data[position + 1] == '/') {
                            advance(2); // Skip '*/'
                            closed = true;
                            break;
                        }
                        advance();
                    }

                    if (!closed) {
                        throw Exceptions::LexerError("Unterminated multi-line comment", commentStart);
                    }
                    continue;
                }
            }

            // Capture location for the token starting here
            SourceLocation tokenLocation{ filename, line, column };

            // check if its a standard alphabetic character or an _
            if (std::isalpha(current) || current == '_') {
                std::string value;

                while (position < data.size() &&
                       (std::isalnum(data[position]) || data[position] == '_')) {
                    value += data[position];
                    advance();
                }

                Token token;
                token.Location = tokenLocation;

                if (value == "import") {
                    token.Type = Token::TokenType::KeywordImport;
                } else if (value == "export") {
                    token.Type = Token::TokenType::KeywordExport;
                } else if (value == "namespace") {
                    token.Type = Token::TokenType::KeywordNamespace;
                } else if (value == "class") {
                    token.Type = Token::TokenType::KeywordClass;
                } else if (value == "struct") {
                    token.Type = Token::TokenType::KeywordStruct;
                } else if (value == "const"){
                    token.Type = Token::TokenType::KeywordConst;
                }else if (value == "auto") {
                    token.Type = Token::TokenType::KeywordAuto;
                } else {
                    token.Type = Token::TokenType::Identifier;
                }

                token.Value = value;
                tokens.push_back(token);
                continue;
            }

            // Handle numbers integers, floats, scientific notation, and alternative radix literals
            if (std::isdigit(current) || 
                (current == '.' && position + 1 < data.size() && std::isdigit(data[position + 1]))) {

                std::string value;

                // Alternative radix literals: 0x, 0b, 0o, 0d
                if (current == '0' && position + 1 < data.size()) {
                    char radix = data[position + 1];

                    if (radix == 'x' || radix == 'X' ||
                        radix == 'b' || radix == 'B' ||
                        radix == 'o' || radix == 'O' ||
                        radix == 'd' || radix == 'D') {

                        value += current;
                        value += radix;
                        advance(2);

                        while (position < data.size() && std::isalnum(data[position])) {
                            value += data[position];
                            advance();
                        }

                        Token token;
                        token.Location = tokenLocation;
                        token.Type = Token::TokenType::Number;
                        token.Value = value;
                        tokens.push_back(token);
                        continue;
                    }
                }

                bool hasDecimal = false;
                bool hasExponent = false;

                // Integer/fraction part
                while (position < data.size()) {
                    char c = data[position];

                    if (std::isdigit(c)) {
                        value += c;
                        advance();
                        continue;
                    }

                    // Decimal point
                    if (c == '.' && !hasDecimal && !hasExponent) {
                        hasDecimal = true;
                        value += c;
                        advance();
                        continue;
                    }

                    break;
                }

                // Scientific notation: e / E
                if (position < data.size() &&
                    (data[position] == 'e' || data[position] == 'E')) {

                    hasExponent = true;
                    value += data[position];
                    advance();

                    // Optional exponent sign
                    if (position < data.size() &&
                        (data[position] == '+' || data[position] == '-')) {

                        value += data[position];
                        advance();
                    }

                    bool hasExponentDigits = false;

                    while (position < data.size() && std::isdigit(data[position])) {
                        hasExponentDigits = true;
                        value += data[position];
                        advance();
                    }

                    if (!hasExponentDigits) {
                        throw Exceptions::LexerError("Invalid exponent in number literal", tokenLocation);
                    }
                }

                Token token;
                token.Location = tokenLocation;
                token.Type = Token::TokenType::Number;
                token.Value = value;
                tokens.push_back(token);
                continue;
            }

            // Heredoc """, Normal Strings "", and Chars ''
            if (current == '"' || current == '\'') {
                char quoteChar = current;
                bool isHeredoc = false;
                std::string value;

                // Check for multi-line Heredoc string (""")
                if (quoteChar == '"' && position + 2 < data.size() && data[position + 1] == '"' && data[position + 2] == '"') {
                    isHeredoc = true;
                    advance(3); // skip opening """

                    bool closed = false;
                    while (position < data.size()) {
                        if (data[position] == '"' && position + 2 < data.size() && data[position + 1] == '"' && data[position + 2] == '"') {
                            advance(3); // skip closing """
                            closed = true;
                            break;
                        }
                        value += data[position];
                        advance();
                    }

                    if (!closed) {
                        throw Exceptions::LexerError("Unterminated heredoc string literal", tokenLocation);
                    }
                } 
                // Normal single-line string or character literal
                else {
                    advance(); // skip opening quote

                    bool closed = false;
                    while (position < data.size()) {
                        if (data[position] == '\n') {
                            throw Exceptions::LexerError(quoteChar == '"' ? "Newline in constant string literal" : "Newline in character literal", tokenLocation);
                        }

                        // Handle escape sequences (e.g., \", \', \\)
                        if (data[position] == '\\') {
                            if (position + 1 >= data.size()) {
                                throw Exceptions::LexerError("Trailing escape sequence at end of file", tokenLocation);
                            }
                            value += data[position];     // add backslash
                            value += data[position + 1]; // add escaped character
                            advance(2);
                            continue;
                        }

                        if (data[position] == quoteChar) {
                            advance(); // skip closing quote
                            closed = true;
                            break;
                        }

                        value += data[position];
                        advance();
                    }

                    if (!closed) {
                        throw Exceptions::LexerError(quoteChar == '"' ? "Unterminated string literal" : "Unterminated character literal", tokenLocation);
                    }
                }

                Token token;
                token.Location = tokenLocation;
                token.Type = (quoteChar == '"') ? Token::TokenType::String : Token::TokenType::Symbol;
                token.Value = isHeredoc
                    ? "\"\"\"" + value + "\"\"\""
                    : std::string(1, quoteChar) + value + std::string(1, quoteChar);
                tokens.push_back(token);
                continue;
            }

            // Multi-Character Operators combined into unified single symbols
            if (position + 1 < data.size()) {
                std::string op = data.substr(position, 2);
                Token::TokenType matchedType = Token::TokenType::Symbol;
                bool isMultiOp = false;

                if (op == "::") { matchedType = Token::TokenType::ScopeResolution; isMultiOp = true; }
                else if (op == "==") { matchedType = Token::TokenType::Symbol; isMultiOp = true; }
                else if (op == "!=") { matchedType = Token::TokenType::Symbol; isMultiOp = true; }
                else if (op == "<=") { matchedType = Token::TokenType::Symbol; isMultiOp = true; }
                else if (op == ">=") { matchedType = Token::TokenType::Symbol; isMultiOp = true; }
                else if (op == "&&") { matchedType = Token::TokenType::Symbol; isMultiOp = true; }
                else if (op == "||") { matchedType = Token::TokenType::Symbol; isMultiOp = true; }
                else if (op == "+=") { matchedType = Token::TokenType::Symbol; isMultiOp = true; }
                else if (op == "-=") { matchedType = Token::TokenType::Symbol; isMultiOp = true; }
                else if (op == "++") { matchedType = Token::TokenType::Symbol; isMultiOp = true; }
                else if (op == "--") { matchedType = Token::TokenType::Symbol; isMultiOp = true; }

                if (isMultiOp) {
                    Token token;
                    token.Location = tokenLocation;
                    token.Type = matchedType;
                    token.Value = op;
                    tokens.push_back(token);
                    advance(2);
                    continue;
                }
            }

            // standard Single-Character Symbols
            Token token;
            token.Location = tokenLocation;
            
            switch (current) {
                case '{':
                    token.Type = Token::TokenType::OpenBrace;
                    break;
                case '}':
                    token.Type = Token::TokenType::CloseBrace;
                    break;
                case '(':
                    token.Type = Token::TokenType::OpenParen;
                    break;
                case ')':
                    token.Type = Token::TokenType::CloseParen;
                    break;
                case '[':
                    token.Type = Token::TokenType::OpenBracket;
                    break;
                case ']':
                    token.Type = Token::TokenType::CloseBracket;
                    break;
                case '@':
                    token.Type = Token::TokenType::Handle;
                    break;
                case '&':
                    token.Type = Token::TokenType::Reference;
                    break;
                case ';':
                    token.Type = Token::TokenType::Semicolon;
                    break;
                case ',':
                    token.Type = Token::TokenType::Comma;
                    break;
                case '=':
                    token.Type = Token::TokenType::Assignment;
                    break;
                default:
                    token.Type = Token::TokenType::Symbol;
                    break;
            }

            token.Value = std::string(1, current);

            tokens.push_back(token);
            advance();
        }

        Token end;
        end.Type = Token::TokenType::EndOfFile;
        end.Location = SourceLocation{ filename, line, column };

        tokens.push_back(end);
        return tokens;
    }
}
