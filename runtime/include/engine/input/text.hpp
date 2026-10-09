#pragma once

#include <string>
#include "engine/common/window.hpp"

namespace CE::Input {
    class TextInput {
        public: 
            TextInput(int window_id, Common::Window& window);

            void Begin();
            void End();

            void Update();

            // This is text/characters the user has actually typed, for this frame 
            const std::string& GetFinalText() const;
            // Gives temp text the IME text that the user is working on
            const std::string& GetEditingText() const;
        private:
            int mWindowID;
            Common::Window& mWindow;
            std::string mFinalText;
            std::string mEditingText;
    };
}