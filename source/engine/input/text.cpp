#include "engine/input/text.hpp"
#include <string>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include "engine/common/sdl_events.hpp"
#include "engine/common/window.hpp"

namespace CE::Input {
    TextInput::TextInput(int window_id, Common::Window& window) : mWindow(window) {
        mWindowID = window_id;
    }

    void TextInput::Begin() {
        mFinalText.clear();
        mEditingText.clear();
        SDL_StartTextInput(mWindow.GetWindow());
    }

    void TextInput::End() {
        SDL_StopTextInput(mWindow.GetWindow());
    }

    const std::string& TextInput::GetFinalText() const  {
        return mFinalText;
    }

    const std::string& TextInput::GetEditingText() const {
        return mEditingText;
    }

    void TextInput::Update() {
        mFinalText.clear();
        auto indices = CE::SDL_Events::GetWindowEventIndices(mWindowID);

        for (size_t idx : indices) {
            const SDL_Event& event = CE::SDL_Events::gEvents[idx]; 

            if (event.type == SDL_EVENT_TEXT_INPUT) {
                mFinalText += event.text.text;
            } else if (event.type == SDL_EVENT_TEXT_EDITING) {
                mEditingText = event.edit.text;
            }
        }
    }
}