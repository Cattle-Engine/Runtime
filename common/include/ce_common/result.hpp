#pragma once

#include <variant>
#include <utility>
#include <stdexcept>
#include <type_traits>
#include <functional>

#include "ce_common/error.hpp"

namespace CE {
    // Result<T>: value types
    template<typename T>
    class [[nodiscard("Result must not be discarded!")]] Result {
        static_assert(!std::is_reference_v<T>,
            "Reference results are handled by the Result<T&> specialization");

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

            T&& operator*() && {
                return std::move(*this).Value();
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

    // Result<T&>: reference results
    template<typename T>
    class [[nodiscard("Result must not be discarded!")]] Result<T&> {
        public:
            Result(T& value)
                : mData(std::ref(value)) {}

            Result(const Error& error)
                : mData(error) {}

            Result(Error&& error)
                : mData(std::move(error)) {}

            bool HasValue() const noexcept {
                return std::holds_alternative<std::reference_wrapper<T>>(mData);
            }

            explicit operator bool() const noexcept {
                return HasValue();
            }

            // A Result<T&> is a handle to an existing object, so constness of the
            // Result itself does not propagate to the referenced object.
            T& Value() const {
                return std::get<std::reference_wrapper<T>>(mData).get();
            }

            T& operator*() const {
                return Value();
            }

            T* operator->() const {
                return &Value();
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
            std::variant<std::reference_wrapper<T>, Error> mData;
    };

    // Result<void>
    template<>
    class [[nodiscard("Result must not be discarded!")]] Result<void> {
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