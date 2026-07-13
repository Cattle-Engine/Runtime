#pragma once

#include <exception>
#include <string>
#include <utility>
#include <format>

#include "engine/scripting/private/modules.hpp"

namespace CE::Scripting::Impl::Exceptions {
    class LexerError : public std::exception {
        public:
            LexerError(std::string message, SourceLocation location)
                : mMessage(std::move(message)),
                mLocation(std::move(location)),
                mWhat(std::format("{}, at file: {}, line: {}, column: {}",
                    mMessage, mLocation.File, mLocation.Line, mLocation.Column))
            {}
            
            const char* what() const noexcept override {
                return mWhat.c_str();
            }
        private:
            std::string mMessage;
            SourceLocation mLocation;
            std::string mWhat;
    };

    class ParserError : public std::exception {
        public:
            ParserError(std::string message, SourceLocation location)
                : mMessage(std::move(message)),
                mLocation(std::move(location)),
                mWhat(std::format("{}, at file: {}, line: {}, column: {}",
                    mMessage, mLocation.File, mLocation.Line, mLocation.Column))
            {}
            
            const char* what() const noexcept override {
                return mWhat.c_str();
            }
        private:
            std::string mMessage;
            SourceLocation mLocation;
            std::string mWhat;
    };
    
    class SemanticError : public std::exception {
    public:
        SemanticError(std::string message, SourceLocation location)
        : mMessage(std::move(message)),
        mLocation(std::move(location)),
        mWhat(std::format("{}, at file: {}, line: {}, column: {}",
                          mMessage, mLocation.File, mLocation.Line, mLocation.Column))
        {}
        
        const char* what() const noexcept override {
            return mWhat.c_str();
        }
    private:
        std::string mMessage;
        SourceLocation mLocation;
        std::string mWhat;
    };
}