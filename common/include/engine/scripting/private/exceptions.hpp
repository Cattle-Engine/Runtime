#pragma once

#include <exception>
#include <format>
#include <string>
#include <utility>

#include "engine/scripting/private/modules.hpp"

namespace CE::Scripting::Impl::Exceptions {
    class LexerError : public std::exception {
      public:
        LexerError(std::string message, SourceLocation location)
            : mMessage(std::move(message)),
              mLocation(std::move(location)),
              mWhat(std::format("{}:{}:{}: error: {}", DisplayFile(mLocation), mLocation.Line, mLocation.Column,
                                mMessage)) {}

        const char* what() const noexcept override {
            return mWhat.c_str();
        }

      private:
        static const std::string& DisplayFile(const SourceLocation& location) {
            static const std::string unknown_file = "<script>";
            return location.File.empty() ? unknown_file : location.File;
        }

        std::string mMessage;
        SourceLocation mLocation;
        std::string mWhat;
    };

    class ParserError : public std::exception {
      public:
        ParserError(std::string message, SourceLocation location)
            : mMessage(std::move(message)),
              mLocation(std::move(location)),
              mWhat(std::format("{}:{}:{}: error: {}", DisplayFile(mLocation), mLocation.Line, mLocation.Column,
                                mMessage)) {}

        const char* what() const noexcept override {
            return mWhat.c_str();
        }

      private:
        static const std::string& DisplayFile(const SourceLocation& location) {
            static const std::string unknown_file = "<script>";
            return location.File.empty() ? unknown_file : location.File;
        }

        std::string mMessage;
        SourceLocation mLocation;
        std::string mWhat;
    };

    class SemanticError : public std::exception {
      public:
        SemanticError(std::string message, SourceLocation location)
            : mMessage(std::move(message)),
              mLocation(std::move(location)),
              mWhat(std::format("{}:{}:{}: error: {}", DisplayFile(mLocation), mLocation.Line, mLocation.Column,
                                mMessage)) {}

        const char* what() const noexcept override {
            return mWhat.c_str();
        }

      private:
        static const std::string& DisplayFile(const SourceLocation& location) {
            static const std::string unknown_file = "<script>";
            return location.File.empty() ? unknown_file : location.File;
        }

        std::string mMessage;
        SourceLocation mLocation;
        std::string mWhat;
    };
} // namespace CE::Scripting::Impl::Exceptions
