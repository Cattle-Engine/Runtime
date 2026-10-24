
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <typeindex>
#include <type_traits>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/accs/actor.hpp"
#include "engine/accs/component.hpp"

namespace CE::ACCS {
    class World {
        public:
            World() = default;

            World(const World&) = delete;
            World& operator=(const World&) = delete;

            ActorHandle CreateActor();
            bool DestroyActor(ActorHandle actor);

            template <typename T, typename... Args>
            ComponentID AddComponent(ActorHandle actorHandle, Args&&... args) {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");

                Actor* actor = GetActor(actorHandle);
                if (!actor) {
                    throw std::invalid_argument("Invalid actor handle");
                }

                const std::type_index type(typeid(T));

                if (actor->component_lookup.contains(type)) {
                    throw std::logic_error(
                        "Actor already has this component type");
                }

                auto component = std::make_shared<T>(
                    std::forward<Args>(args)...);

                ComponentID id = AllocateComponentID();
                auto& slot = mComponents[id.index];

                bool component_added = false;
                bool lookup_added = false;

                try {
                    slot.entry.emplace(ComponentEntry{
                        id.generation,
                        actorHandle,
                        type,
                        std::move(component)
                    });

                    actor->components.emplace_back(id);
                    component_added = true;

                    auto result = actor->component_lookup.emplace(type, id);
                    if (!result.second) {
                        throw std::logic_error(
                            "Actor already has this component type");
                    }

                    lookup_added = true;
                } catch (...) {
                    if (lookup_added) {
                        actor->component_lookup.erase(type);
                    }

                    if (component_added) {
                        for (auto it = actor->components.begin();
                            it != actor->components.end(); ++it) {
                            if (*it == id) {
                                actor->components.erase(it);
                                break;
                            }
                        }
                    }

                    slot.entry.reset();
                    ReleaseComponentID(id);
                    throw;
                }

                return id;
            }

            template <typename T>
            T* GetComponent(ComponentID id) {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");

                ComponentEntry* entry = GetComponentEntry(id);

                if (!entry ||
                    entry->type != std::type_index(typeid(T))) {
                    return nullptr;
                }

                return static_cast<T*>(entry->instance.get());
            }

            template <typename T>
            const T* GetComponent(ComponentID id) const {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");

                const ComponentEntry* entry = GetComponentEntry(id);

                if (!entry ||
                    entry->type != std::type_index(typeid(T))) {
                    return nullptr;
                }

                return static_cast<const T*>(entry->instance.get());
            }

            bool HasComponent(ComponentID id) const {
                return GetComponentEntry(id) != nullptr;
            }

            bool RemoveComponent(ComponentID id) {
                ComponentEntry* entry = GetComponentEntry(id);

                if (!entry) {
                    return false;
                }

                Actor* actor = GetActor(entry->owner);

                if (actor) {
                    actor->component_lookup.erase(entry->type);

                    for (auto it = actor->components.begin();
                        it != actor->components.end(); ++it) {
                        if (*it == id) {
                            actor->components.erase(it);
                            break;
                        }
                    }
                }

                mComponents[id.index].entry.reset();
                ReleaseComponentID(id);

                return true;
            }

            template <typename T>
            bool IsComponentType(ComponentID id) const {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");

                const ComponentEntry* entry = GetComponentEntry(id);

                return entry &&
                    entry->type == std::type_index(typeid(T));
            }

            template <typename T>
            T* GetComponent(ActorHandle actorHandle) {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");

                Actor* actor = GetActor(actorHandle);

                return actor ? GetComponentFromActor<T>(*actor) : nullptr;
            }

            template <typename T>
            const T* GetComponent(ActorHandle actorHandle) const {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");

                const Actor* actor = GetActor(actorHandle);

                return actor ? GetComponentFromActor<T>(*actor) : nullptr;
            }

            template <typename T>
            bool HasComponent(ActorHandle actorHandle) const {
                return GetComponent<T>(actorHandle) != nullptr;
            }

            // Iterates over every actor that has all requested components.
            // Components are passed to the callback in template argument order.
            template <typename... Components, typename Func>
            void Each(Func&& func) {
                static_assert(sizeof...(Components) > 0,
                    "At least one component type is required");

                static_assert(
                    (std::is_base_of_v<IComponent, Components> && ...),
                    "All types must inherit from IComponent");

                for (auto& slot : mActorSlots) {
                    if (!slot.actor) {
                        continue;
                    }

                    Actor& actor = *slot.actor;

                    auto components =
                        std::tuple<Components*...>{
                            GetComponentFromActor<Components>(actor)...
                        };

                    const bool matches = std::apply(
                        [](auto*... ptrs) {
                            return ((ptrs != nullptr) && ...);
                        },
                        components);

                    if (!matches) {
                        continue;
                    }

                    std::apply(
                        [&func](auto*... ptrs) {
                            std::invoke(func, *ptrs...);
                        },
                        components);
                }
            }

            // Const iteration provides read-only component access.
            template <typename... Components, typename Func>
            void Each(Func&& func) const {
                static_assert(sizeof...(Components) > 0,
                    "At least one component type is required");

                static_assert(
                    (std::is_base_of_v<IComponent, Components> && ...),
                    "All types must inherit from IComponent");

                for (const auto& slot : mActorSlots) {
                    if (!slot.actor) {
                        continue;
                    }

                    const Actor& actor = *slot.actor;

                    auto components =
                        std::tuple<const Components*...>{
                            GetComponentFromActor<Components>(actor)...
                        };

                    const bool matches = std::apply(
                        [](auto*... ptrs) {
                            return ((ptrs != nullptr) && ...);
                        },
                        components);

                    if (!matches) {
                        continue;
                    }

                    std::apply(
                        [&func](auto*... ptrs) {
                            std::invoke(func, *ptrs...);
                        },
                        components);
                }
            }

        private:
            struct ActorSlot {
                std::unique_ptr<Actor> actor;
                uint32_t generation = 0;
                uint32_t next_free = kInvalidActorHandleIndex;
            };

            struct ComponentEntry {
                uint32_t generation;
                ActorHandle owner;
                std::type_index type;
                std::shared_ptr<IComponent> instance;
            };

            struct ComponentSlot {
                std::optional<ComponentEntry> entry;
                uint32_t generation = 0;
                uint32_t next_free = kInvalidComponentIndex;
            };

            Actor* GetActor(ActorHandle handle);
            const Actor* GetActor(ActorHandle handle) const;

            ComponentID AllocateComponentID();
            void ReleaseComponentID(ComponentID id);

            ComponentEntry* GetComponentEntry(ComponentID id) {
                if (!id.IsValid() || id.index >= mComponents.size()) {
                    return nullptr;
                }

                auto& slot = mComponents[id.index];

                if (!slot.entry ||
                    slot.generation != id.generation ||
                    slot.entry->generation != id.generation) {
                    return nullptr;
                }

                return &*slot.entry;
            }

            const ComponentEntry* GetComponentEntry(ComponentID id) const {
                if (!id.IsValid() || id.index >= mComponents.size()) {
                    return nullptr;
                }

                const auto& slot = mComponents[id.index];

                if (!slot.entry ||
                    slot.generation != id.generation ||
                    slot.entry->generation != id.generation) {
                    return nullptr;
                }

                return &*slot.entry;
            }

            template <typename T>
            T* GetComponentFromActor(Actor& actor) {
                const auto it = actor.component_lookup.find(
                    std::type_index(typeid(T)));

                if (it == actor.component_lookup.end()) {
                    return nullptr;
                }

                return GetComponent<T>(it->second);
            }

            template <typename T>
            const T* GetComponentFromActor(const Actor& actor) const {
                const auto it = actor.component_lookup.find(
                    std::type_index(typeid(T)));

                if (it == actor.component_lookup.end()) {
                    return nullptr;
                }

                return GetComponent<T>(it->second);
            }

            std::vector<ActorSlot> mActorSlots;
            uint32_t mFreeHead = kInvalidActorHandleIndex;

            std::vector<ComponentSlot> mComponents;
            uint32_t mComponentFreeHead = kInvalidComponentIndex;
    };
}
