#pragma once

#include <cstdint>
#include <functional>
#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/accs/actor.hpp"
#include "engine/accs/component.hpp"
#include "engine/accs/component_registry.hpp"

namespace CE::ACCS {
    class World {
        public:
            World() = default;

            World(const World&) = delete;
            World& operator=(const World&) = delete;

            template <typename T>
            bool RegisterComponent(std::string name) {
                return mRegistry.Register<T>(std::move(name));
            }

            ComponentRegistry& GetRegistry() { return mRegistry; }
            const ComponentRegistry& GetRegistry() const { return mRegistry; }

            ActorHandle CreateActor();
            bool DestroyActor(ActorHandle actor);

            std::vector<ActorHandle> GetActors() const;
            std::vector<ComponentID> GetComponents(ActorHandle actorHandle) const;

            ComponentID AddComponent(ActorHandle actor, const std::string& name);
            ComponentID AddComponent(ActorHandle actorHandle, ComponentTypeKey type, std::shared_ptr<IComponent> component);

            IComponent* GetComponent(ActorHandle actor, const std::string& name);
            const IComponent* GetComponent(ActorHandle actor, const std::string& name) const;
            bool HasComponent(ActorHandle actor, const std::string& name) const;

            // Registered name of a live component, or nullptr.
            const std::string* GetComponentName(ComponentID id) const {
                const ComponentEntry* entry = GetComponentEntry(id);
                if (!entry) {
                    return nullptr;
                }

                const ComponentTypeInfo* info = mRegistry.Find(entry->type);
                return info ? &info->name : nullptr;
            }

            template <typename T, typename... Args>
            ComponentID AddComponent(ActorHandle actorHandle, Args&&... args) {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");

                if (!mRegistry.Find(GetComponentTypeKey<T>())) {
                    throw std::logic_error(
                        "Component type not registered with RegisterComponent<T>()");
                }

                if (!GetActor(actorHandle)) {
                    throw std::invalid_argument("Invalid actor handle");
                }

                return AddComponentRaw(
                    actorHandle,
                    GetComponentTypeKey<T>(),
                    std::make_shared<T>(std::forward<Args>(args)...));
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

            IComponent* GetComponent(ComponentID id) {
                ComponentEntry* entry = GetComponentEntry(id);
                return entry ? entry->instance.get() : nullptr;
            }

            const IComponent* GetComponent(ComponentID id) const {
                const ComponentEntry* entry = GetComponentEntry(id);
                return entry ? entry->instance.get() : nullptr;
            }

            template <typename T>
            T* GetComponent(ComponentID id) {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");

                ComponentEntry* entry = GetComponentEntry(id);

                if (!entry ||
                    entry->type != GetComponentTypeKey<T>()) {
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
                    entry->type != GetComponentTypeKey<T>()) {
                    return nullptr;
                }

                return static_cast<const T*>(entry->instance.get());
            }

            bool HasComponent(ComponentID id) const {
                return GetComponentEntry(id) != nullptr;
            }

            template <typename T>
            bool IsComponentType(ComponentID id) const {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");

                const ComponentEntry* entry = GetComponentEntry(id);

                return entry &&
                    entry->type == GetComponentTypeKey<T>();
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

            bool SetActorName(ActorHandle handle, std::string name) {
                Actor* actor = GetActor(handle);
                if (!actor) return false;
                actor->name = std::move(name);
                return true;
            }

            // nullptr if the handle is invalid.
            const std::string* GetActorName(ActorHandle handle) const {
                const Actor* actor = GetActor(handle);
                return actor ? &actor->name : nullptr;
            }

            bool SetActorActive(ActorHandle handle, bool active) {
                Actor* actor = GetActor(handle);
                if (!actor) return false;
                actor->active = active;
                return true;
            }

            // false for an invalid handle.
            bool IsActorActive(ActorHandle handle) const {
                const Actor* actor = GetActor(handle);
                return actor && actor->active;
            }

            // false if the handle is invalid, the tag is empty, or it's already present.
            bool AddActorTag(ActorHandle handle, std::string tag) {
                Actor* actor = GetActor(handle);
                if (!actor || tag.empty()) return false;
                if (std::find(actor->tags.begin(), actor->tags.end(), tag) != actor->tags.end()) {
                    return false;
                }
                actor->tags.push_back(std::move(tag));
                return true;
            }

            bool RemoveActorTag(ActorHandle handle, const std::string& tag) {
                Actor* actor = GetActor(handle);
                if (!actor) return false;
                auto it = std::find(actor->tags.begin(), actor->tags.end(), tag);
                if (it == actor->tags.end()) return false;
                actor->tags.erase(it);
                return true;
            }

            bool HasActorTag(ActorHandle handle, const std::string& tag) const {
                const Actor* actor = GetActor(handle);
                return actor &&
                    std::find(actor->tags.begin(), actor->tags.end(), tag) != actor->tags.end();
            }

            // nullptr if the handle is invalid.
            const std::vector<std::string>* GetActorTags(ActorHandle handle) const {
                const Actor* actor = GetActor(handle);
                return actor ? &actor->tags : nullptr;
            }

            bool IsActorAlive(ActorHandle handle) const { return GetActor(handle) != nullptr; }

            ComponentID AddComponentInstance(ActorHandle actor, const std::string& name, std::shared_ptr<IComponent> instance);

        private:
            struct ActorSlot {
                std::unique_ptr<Actor> actor;
                uint32_t generation = 0;
                uint32_t next_free = kInvalidActorHandleIndex;
            };

            struct ComponentEntry {
                uint32_t generation;
                ActorHandle owner;
                ComponentTypeKey type;
                std::shared_ptr<IComponent> instance;
            };

            struct ComponentSlot {
                std::optional<ComponentEntry> entry;
                uint32_t generation = 0;
                uint32_t next_free = kInvalidComponentIndex;
            };

            Actor* GetActor(ActorHandle handle);
            const Actor* GetActor(ActorHandle handle) const;

            ComponentID AddComponentRaw(
                ActorHandle actorHandle,
                ComponentTypeKey type,
                std::shared_ptr<IComponent> component);

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
                const auto it = actor.component_lookup.find(GetComponentTypeKey<T>());

                if (it == actor.component_lookup.end()) {
                    return nullptr;
                }

                return GetComponent<T>(it->second);
            }

            template <typename T>
            const T* GetComponentFromActor(const Actor& actor) const {
                const auto it = actor.component_lookup.find(GetComponentTypeKey<T>());

                if (it == actor.component_lookup.end()) {
                    return nullptr;
                }

                return GetComponent<T>(it->second);
            }

            ComponentRegistry mRegistry;

            std::vector<ActorSlot> mActorSlots;
            uint32_t mFreeHead = kInvalidActorHandleIndex;

            std::vector<ComponentSlot> mComponents;
            uint32_t mComponentFreeHead = kInvalidComponentIndex;
    };
}
