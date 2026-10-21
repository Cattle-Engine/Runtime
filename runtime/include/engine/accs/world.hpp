#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <typeindex>
#include <type_traits>
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

                auto component = std::make_shared<T>(std::forward<Args>(args)...);
                ComponentID id = AllocateComponentID();
                auto& slot = mComponents[id.index];

                try {
                    slot.entry.emplace(ComponentEntry{
                        id.generation,
                        actorHandle,
                        std::type_index(typeid(T)),
                        std::move(component)
                    });
                    actor->components.emplace_back(id);
                } catch (...) {
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
                if (!entry || entry->type != std::type_index(typeid(T))) {
                    return nullptr;
                }

                return static_cast<T*>(entry->instance.get());
            }

            template <typename T>
            const T* GetComponent(ComponentID id) const {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");

                const ComponentEntry* entry = GetComponentEntry(id);
                if (!entry || entry->type != std::type_index(typeid(T))) {
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
                    for (auto it = actor->components.begin(); it != actor->components.end(); ++it) {
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
                return entry && entry->type == std::type_index(typeid(T));
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
                if (!slot.entry || slot.generation != id.generation) {
                    return nullptr;
                }

                return &*slot.entry;
            }

            const ComponentEntry* GetComponentEntry(ComponentID id) const {
                if (!id.IsValid() || id.index >= mComponents.size()) {
                    return nullptr;
                }

                const auto& slot = mComponents[id.index];
                if (!slot.entry || slot.generation != id.generation) {
                    return nullptr;
                }

                return &*slot.entry;
            }

            std::vector<ActorSlot> mActorSlots;
            uint32_t mFreeHead = kInvalidActorHandleIndex;

            std::vector<ComponentSlot> mComponents;
            uint32_t mComponentFreeHead = kInvalidComponentIndex;
    };
}
