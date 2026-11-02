#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "engine/accs/actor.hpp"
#include "engine/accs/accs.hpp"

namespace CE::ACCS {
    constexpr uint32_t kNullActorRef = UINT32_MAX;

    class WorldSerializer;

    class SaveContext {
        public:
            uint32_t Resolve(ActorHandle h) const {
                auto it = mIds.find(Key(h));
                return it != mIds.end() ? it->second : kNullActorRef;
            }

            const Dependencies& GetDependencies() const {
                return mDependencies;
            }

        private:
            friend class WorldSerializer;

            explicit SaveContext(const Dependencies& dependencies)
                : mDependencies(dependencies) {}

            static uint64_t Key(ActorHandle h) {
                return (uint64_t(h.generation) << 32) | h.index;
            }

            const Dependencies& mDependencies;
            std::unordered_map<uint64_t, uint32_t> mIds;
    };

    class LoadContext {
        public:
            ActorHandle Resolve(uint32_t ref) const {
                return ref < mHandles.size() ? mHandles[ref] : ActorHandle{};
            }

            const Dependencies& GetDependencies() const {
                return mDependencies;
            }

        private:
            friend class WorldSerializer;

            explicit LoadContext(const Dependencies& dependencies)
                : mDependencies(dependencies) {}

            const Dependencies& mDependencies;
            std::vector<ActorHandle> mHandles;
    };
}