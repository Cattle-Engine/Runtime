#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace CE::Common::Containers {
    template <typename T> class NameRegistry {
      public:
        void Add(std::string name, T value) {
            mNames[std::move(name)] = value;
        }

        const T& Get(std::string_view name) const {
            static const T emptyValue{};

            auto it = mNames.find(std::string(name));

            if (it == mNames.end())
                return emptyValue;

            return it->second;
        }

        bool Exists(std::string_view name) const {
            return mNames.contains(std::string(name));
        }

        void Remove(std::string_view name) {
            mNames.erase(std::string(name));
        }

        void Clear() {
            mNames.clear();
        }

      private:
        std::unordered_map<std::string, T> mNames;
    };
} // namespace CE::Common::Containers