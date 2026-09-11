#pragma once

#include <string>

#include <SDL3/SDL.h>

#include "engine/common/fs/vfs.hpp"

namespace CE::Common {
    class Window {
        public:
            enum class WindowMode {
                Fullscreen,
                Borderless,
                Windowed,
                Count
            };

            // size in pixels
            struct WindowSize {
                int w, h;
            };

            // Can throw std::runtime error if failed
            Window(VFS::VFS& vfs, const std::string& window_title, WindowSize size, SDL_WindowFlags flags);
            ~Window();

            SDL_Window* GetWindow();
            
            // hides a window from the taskbar, desktop and hides the window itself.
            void HideWindow(bool hidden);
            bool IsHidden();

            // hides a window and has an icon on the taskbar
            void MinimiseWindow(bool minimised);
            bool IsMinimised();

            /*
                On fullscreen we use a user defined resolution, the same for windowed.
                On borderless window it will ALWAYS be desktop resloution, the same for window resolution.
                I will add the ability to set the renderer resolution
            */
            bool SetWindowMode(WindowMode mode);
            WindowMode GetWindowMode();

            bool SetWindowSize(WindowSize window_size, WindowMode mode);
            WindowSize GetWindowSize(WindowMode mode);

            bool SetWindowTitle(const std::string& title);
            // If no title, returns ""
            std::string GetWindowTitle();

            void SetWindowResizable(bool resizable);
            bool IsWindowResizable();

            bool SetWindowIcon(const std::string& path);
        private:
            SDL_Window* mWindow;
            VFS::VFS& mVFS; 
            WindowMode mWindowMode = WindowMode::Borderless;
            WindowSize mWindowedSize;
            WindowSize mFullscreenSize;
    };
}