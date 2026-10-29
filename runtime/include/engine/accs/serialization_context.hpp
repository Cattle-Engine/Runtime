#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "engine/accs/actor.hpp"

namespace CE::ACCS {
    constexpr uint32_t kNullActorRef = UINT32_MAX;

    class WorldSerializer;

    class SaveContext {
        public:
            uint32_t Resolve(ActorHandle h) const {
                auto it = mIds.find(Key(h));
                return it != mIds.end() ? it->second : kNullActorRef;
            }

        private:
            friend class WorldSerializer;

            static uint64_t Key(ActorHandle h) {
                return (uint64_t(h.generation) << 32) | h.index;
            }

            std::unordered_map<uint64_t, uint32_t> mIds;
    };

    class LoadContext {
        public:
            // Returns a default (invalid) ActorHandle for null/unknown refs.
            ActorHandle Resolve(uint32_t ref) const {
                return ref < mHandles.size() ? mHandles[ref] : ActorHandle{};
            }

        private:
            friend class WorldSerializer;
            std::vector<ActorHandle> mHandles;
    };
}
