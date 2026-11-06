#pragma once

namespace CE::UI {
    class LogWindow {
        public:
        // the window has a close button, this will reshow the window if the user has clicked close
            void SetWindowOpen(bool open) { mWindowOpen = open; };
            void DrawWindow();
        private:
            bool mWindowOpen = false;
    };
}