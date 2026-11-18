
#pragma once

#include <variant>
#include <utility>
#include <stdexcept>
#include <type_traits>

#include "ce_common/error.hpp"

namespace CE {
    template<typename T>
    class Result {
    public:
        Result(const T& value)
            : mData(value) {}

        Result(T&& value)
            : mData(std::move(value)) {}

        Result(const Error& error)
            : mData(error) {}

        Result(Error&& error)
            : mData(std::move(error)) {}

        bool HasValue() const noexcept {
            return std::holds_alternative<T>(mData);
        }

        explicit operator bool() const noexcept {
            return HasValue();
        }

        T& Value() & {
            return std::get<T>(mData);
        }

        const T& Value() const & {
            return std::get<T>(mData);
        }

        T&& Value() && {
            return std::get<T>(std::move(mData));
        }

        Error& GetError() & {
            return std::get<Error>(mData);
        }

        const Error& GetError() const & {
            return std::get<Error>(mData);
        }

        Error&& GetError() && {
            return std::get<Error>(std::move(mData));
        }

        T ValueOr(T fallback) const & {
            return HasValue() ? Value() : std::move(fallback);
        }

        T& operator*() & {
            return Value();
        }

        const T& operator*() const & {
            return Value();
        }

        T* operator->() {
            return &Value();
        }

        const T* operator->() const {
            return &Value();
        }

    private:
        std::variant<T, Error> mData;
    };

    template<>
    class Result<void> {
    public:
        Result() noexcept
            : mData(std::monostate{}) {}

        Result(const Error& error)
            : mData(error) {}

        Result(Error&& error)
            : mData(std::move(error)) {}

        bool HasValue() const noexcept {
            return std::holds_alternative<std::monostate>(mData);
        }

        explicit operator bool() const noexcept {
            return HasValue();
        }

        void Value() const {
            if (!HasValue()) {
                throw std::runtime_error("Result contains an error");
            }
        }

        Error& GetError() & {
            return std::get<Error>(mData);
        }

        const Error& GetError() const & {
            return std::get<Error>(mData);
        }

        Error&& GetError() && {
            return std::get<Error>(std::move(mData));
        }

    private:
        std::variant<std::monostate, Error> mData;
    };

    using Status = Result<void>;
}
