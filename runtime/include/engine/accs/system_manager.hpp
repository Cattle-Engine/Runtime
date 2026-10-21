#pragma once

#include <array>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/accs/system.hpp"

namespace CE::ACCS {
    class SystemManager {
        public:
            template<typename T, typename... Args>
            T& RegisterSystem(FrameUpdateType type, Args&&... args) {
                static_assert(std::is_base_of_v<ISystem, T>,
                    "T must inherit from ISystem");

                auto system = std::make_unique<T>(
                    std::forward<Args>(args)...
                );

                T& ref = *system;

                const auto index = static_cast<std::size_t>(type);
                if (index == 0 || index >= static_cast<std::size_t>(FrameUpdateType::Count)) {
                    throw std::invalid_argument("Invalid frame update type");
                }

                mSystems[index].emplace_back(std::move(system));
                return ref;
            }

            void Update(float dt, FrameUpdateType type, World& world) {
                const auto index = static_cast<std::size_t>(type);
                if (index == 0 || index >= static_cast<std::size_t>(FrameUpdateType::Count)) {
                    return;
                }

                for (auto& system : mSystems[index]) {
                    system->Update(dt, type, world);
                }
            }

        private:
            std::array<
                std::vector<std::unique_ptr<ISystem>>,
                static_cast<std::size_t>(FrameUpdateType::Count)
            > mSystems;
    };
}