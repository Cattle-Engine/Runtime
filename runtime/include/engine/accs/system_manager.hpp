#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/accs/system.hpp"

namespace CE::ACCS {
    class ACCS;

    class SystemManager {
        public:
            SystemManager(ACCS& accs) : mACCS(accs) {}

            template<typename T, typename... Args>
            T& RegisterSystem(FrameUpdateType types, Args&&... args) {
                static_assert(std::is_base_of_v<ISystem, T>,
                    "T must inherit from ISystem");

                if (types == FrameUpdateType::None) {
                    throw std::invalid_argument("System must have at least one update type");
                }

                auto system = std::make_unique<T>(
                    std::forward<Args>(args)...
                );

                T& ref = *system;
                mSystems.emplace_back(std::move(system), types);
                return ref;
            }

            void Update(float dt, FrameUpdateType type, World& world) {
                if (type == FrameUpdateType::None) {
                    return;
                }

                for (auto& entry : mSystems) {
                    if (HasFlag(entry.types, type)) {
                        entry.system->Update(dt, type, world, mACCS);
                    }
                }
            }

        private:
            struct SystemEntry {
                std::unique_ptr<ISystem> system;
                FrameUpdateType types;

                SystemEntry(std::unique_ptr<ISystem> system, FrameUpdateType types)
                    : system(std::move(system)), types(types) {}
            };

            std::vector<SystemEntry> mSystems;
            ACCS& mACCS;
    };
}