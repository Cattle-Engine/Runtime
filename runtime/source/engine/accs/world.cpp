#include "engine/accs/world.hpp"
#include "engine/accs/actor.hpp"

namespace CE::ACCS {
    ActorHandle World::CreateActor() {
        if (mFreeHead != kInvalidActorHandleIndex) {
            const uint32_t index = mFreeHead;
            ActorSlot& slot = mActorSlots[index];

            mFreeHead = slot.next_free;
            slot.next_free = kInvalidActorHandleIndex;

            slot.actor = std::make_unique<Actor>();

            return {index, slot.generation};
        }

        if (mActorSlots.size() >= kInvalidActorHandleIndex) {
            return {};
        }

        const uint32_t index = static_cast<uint32_t>(mActorSlots.size()); // NOLINT
        mActorSlots.emplace_back();

        ActorSlot& slot = mActorSlots.back();
        slot.actor = std::make_unique<Actor>();

        return {index, slot.generation};
    }

    bool World::DestroyActor(ActorHandle actor) {
        Actor* instance = GetActor(actor);
        if (!instance) {
            return false;
        }

        while (!instance->components.empty()) {
            RemoveComponent(instance->components.back());
        }

        ActorSlot& slot = mActorSlots[actor.index];
        slot.actor.reset();

        ++slot.generation;
        if (slot.generation == kInvalidActorHandleIndex) {
            ++slot.generation;
        }

        slot.next_free = mFreeHead;
        mFreeHead = actor.index;

        return true;
    }

    ComponentID World::AddComponentRaw(ActorHandle actorHandle, ComponentTypeKey type, std::shared_ptr<IComponent> component) {
        Actor* actor = GetActor(actorHandle);
        if (!actor) {
            throw std::invalid_argument("Invalid actor handle");
        }
        if (type == nullptr || !component) {
            throw std::invalid_argument("Component type and instance are required");
        }
        if (actor->component_lookup.contains(type)) {
            throw std::logic_error("Actor already has this component type");
        }

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

            if (!actor->component_lookup.emplace(type, id).second) {
                throw std::logic_error("Actor already has this component type");
            }
            lookup_added = true;
        } catch (...) {
            if (lookup_added) {
                actor->component_lookup.erase(type);
            }
            if (component_added) {
                actor->components.pop_back();
            }
            slot.entry.reset();
            ReleaseComponentID(id);
            throw;
        }

        return id;
    }

    std::vector<ActorHandle> World::GetActors() const {
        std::vector<ActorHandle> actors;
        actors.reserve(mActorSlots.size());
        for (uint32_t index = 0; index < mActorSlots.size(); ++index) {
            const ActorSlot& slot = mActorSlots[index];
            if (slot.actor) {
                actors.push_back({index, slot.generation});
            }
        }
        return actors;
    }

    std::vector<ComponentID> World::GetComponents(ActorHandle actorHandle) const {
        const Actor* actor = GetActor(actorHandle);
        return actor ? actor->components : std::vector<ComponentID>{};
    }


    Actor* World::GetActor(ActorHandle handle) {
        if (handle.index >= mActorSlots.size()) {
            return nullptr;
        }

        ActorSlot& slot = mActorSlots[handle.index];

        if (!slot.actor || slot.generation != handle.generation) {
            return nullptr;
        }

        return slot.actor.get();
    }

    const Actor* World::GetActor(ActorHandle handle) const {
        if (handle.index >= mActorSlots.size()) {
            return nullptr;
        }

        const ActorSlot& slot = mActorSlots[handle.index];
        if (!slot.actor || slot.generation != handle.generation) {
            return nullptr;
        }
        return slot.actor.get();
    }

    ComponentID World::AllocateComponentID() {
        if (mComponentFreeHead != kInvalidComponentIndex) {
            const uint32_t index = mComponentFreeHead;
            ComponentSlot& slot = mComponents[index];

            mComponentFreeHead = slot.next_free;
            slot.next_free = kInvalidComponentIndex;

            return ComponentID{index, slot.generation};
        }

        if (mComponents.size() >= kInvalidComponentIndex) {
            throw std::length_error("Component storage exhausted");
        }

        const uint32_t index = static_cast<uint32_t>(mComponents.size());

        mComponents.emplace_back();

        return ComponentID{index, mComponents[index].generation};
    }

    void World::ReleaseComponentID(ComponentID id) {
        if (!id.IsValid() || id.index >= mComponents.size()) {
            return;
        }

        ComponentSlot& slot = mComponents[id.index];

        ++slot.generation;
        if (slot.generation == kInvalidComponentIndex) {
            ++slot.generation;
        }

        slot.next_free = mComponentFreeHead;
        mComponentFreeHead = id.index;
    }

    ComponentID World::AddComponent(ActorHandle handle, const std::string& name) {
        const ComponentTypeInfo* info = mRegistry.Find(name);
        Actor* actor = GetActor(handle);

        if (!info || !actor) {
            return ComponentID{};
        }

        if (actor->component_lookup.find(info->key) != actor->component_lookup.end()) {
            return ComponentID{};
        }

        return AddComponentRaw(handle, info->key, info->create());
    }

    ComponentID World::AddComponent(ActorHandle actorHandle, ComponentTypeKey type, std::shared_ptr<IComponent> component) {
        Actor* actor = GetActor(actorHandle);
        if (!actor) {
            throw std::invalid_argument("Invalid actor handle");
        }
        if (type == nullptr || !component) {
            throw std::invalid_argument("Component type and instance are required");
        }
        if (actor->component_lookup.contains(type)) {
            throw std::logic_error("Actor already has this component type");
        }

        return AddComponentRaw(actorHandle, type, std::move(component));
    }

    IComponent* World::GetComponent(ActorHandle handle, const std::string& name) {
        const ComponentTypeInfo* info = mRegistry.Find(name);
        Actor* actor = GetActor(handle);

        if (!info || !actor) {
            return nullptr;
        }

        auto it = actor->component_lookup.find(info->key);
        return it == actor->component_lookup.end() ? nullptr : GetComponent(it->second);
    }

    const IComponent* World::GetComponent(ActorHandle handle, const std::string& name) const {
        const ComponentTypeInfo* info = mRegistry.Find(name);
        const Actor* actor = GetActor(handle);

        if (!info || !actor) {
            return nullptr;
        }

        auto it = actor->component_lookup.find(info->key);
        return it == actor->component_lookup.end() ? nullptr : GetComponent(it->second);
    }

    bool World::HasComponent(ActorHandle handle, const std::string& name) const {
        return GetComponent(handle, name) != nullptr;
    }

    ComponentID World::AddComponentInstance(ActorHandle handle, const std::string& name, std::shared_ptr<IComponent> instance) {
        const ComponentTypeInfo* info = mRegistry.Find(name);
        Actor* actor = GetActor(handle);

        if (!info || !actor || !instance) {
            return ComponentID{};
        }
        if (actor->component_lookup.find(info->key) != actor->component_lookup.end()) {
            return ComponentID{};
        }

        return AddComponentRaw(handle, info->key, std::move(instance));
    }
}
