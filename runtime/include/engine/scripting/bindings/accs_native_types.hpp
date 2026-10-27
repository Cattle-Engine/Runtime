
#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "engine/accs/accs.hpp"

namespace CE::Scripting::Bindings {
    class NativeTypeRegistry {
    public:
        using Factory = std::function<ACCS::ComponentID(
            ACCS::World&,
            ACCS::ActorHandle
        )>;

        template<typename T>
        bool Register(std::string_view name) {
            static_assert(
                std::is_base_of_v<ACCS::IComponent, T>,
                "T must inherit from ACCS::IComponent"
            );
            static_assert(
                std::is_default_constructible_v<T>,
                "T must be default constructible"
            );

            std::string key(name);

            if (mFactories.contains(key)) {
                return false;
            }

            mFactories.emplace(
                std::move(key),
                [](ACCS::World& world, ACCS::ActorHandle actor) {
                    return world.AddComponent<T>(actor);
                }
            );

            return true;
        }

        ACCS::ComponentID Create(
            std::string_view name,
            ACCS::World& world,
            ACCS::ActorHandle actor
        ) const {
            auto it = mFactories.find(std::string(name));
            if (it == mFactories.end()) {
                return {};
            }

            return it->second(world, actor);
        }

        bool Contains(std::string_view name) const {
            return mFactories.contains(std::string(name));
        }

    private:
        std::unordered_map<std::string, Factory> mFactories;
    };
}
