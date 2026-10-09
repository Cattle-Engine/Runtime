#include <utility>
#include "engine/common/core/game_state.hpp"

namespace CE::Core::GameState {
    GameStateManager::GameStateManager(EventBus& eventbus) : mEventBus(eventbus) {}

    const std::string& GameStateManager::GetState() const {
        return mCurrentState;
    }

    bool GameStateManager::IsState(std::string_view state) const {
        return mCurrentState == state;
    }

    void GameStateManager::Emit(const std::string& eventName) const {
        mEventBus.Emit(mCurrentState, eventName);
    }

    void GameStateManager::ChangeState(std::string state) {
        std::string next = state.empty() ? "None" : state;

        if (next == mCurrentState)
            return;

        if (mCurrentState != "None") {
            mEventBus.Emit(StateExitEvent{mCurrentState});
            mEventBus.Emit(mCurrentState, "STATE_EXIT");
        }

        std::string old = mCurrentState;
        mCurrentState = next;

        mEventBus.Emit(StateEnterEvent{mCurrentState});
        mEventBus.Emit(mCurrentState, "STATE_ENTER");

        mEventBus.Emit(StateChangedEvent{old, mCurrentState});
    }

    int GameStateManager::Subscribe(
        const std::string& state,
        const std::string& eventName,
        EventBus::StateHandler handler
    ) {
      return mEventBus.Subscribe(state, eventName, std::move(handler));
    }

    void GameStateManager::Unsubscribe(
        const std::string& state,
        const std::string& eventName,
        int id
    ) {
        mEventBus.Unsubscribe(state, eventName, id);
    }
} // namespace CE::Core::GameState
