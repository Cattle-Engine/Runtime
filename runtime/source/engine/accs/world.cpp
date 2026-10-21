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
}