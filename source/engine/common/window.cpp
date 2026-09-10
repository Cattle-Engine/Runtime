#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <stdexcept>

#include "engine/common/window.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Common {
    Window::Window(VFS::VFS& vfs, const std::string& window_title, WindowSize size, SDL_WindowFlags flags) : mVFS(vfs) {
        mWindow = SDL_CreateWindow(window_title.c_str(), size.w, size.h, flags);
        if (!mWindow) {
            CE_LOG(LogLevel::Error, "[Window] Failed to create window: {}", SDL_GetError());
            throw std::runtime_error("Failed to create window");
        }
    }

    Window::~Window() {
        SDL_DestroyWindow(mWindow);
    }

    SDL_Window* Window::GetWindow() {
        return mWindow;
    }

    void Window::HideWindow(bool hidden) {
        SDL_WindowFlags window_flags = SDL_GetWindowFlags(mWindow);

        if (window_flags & SDL_WINDOW_HIDDEN && hidden == true) {
            CE_LOG(LogLevel::Warn, "[Window] Window is already hidden. It cannot be hidden again!");
            return;
        } else if (!(window_flags & SDL_WINDOW_HIDDEN) && hidden == false) {
            CE_LOG(LogLevel::Warn, "[Window] Window is unhidden. It cannot be unhidden");
            return;
        }

        if (hidden == true) {
            SDL_HideWindow(mWindow);
        } else {
            SDL_ShowWindow(mWindow);
        }
    }

    bool Window::IsHidden() {
        SDL_WindowFlags window_flags = SDL_GetWindowFlags(mWindow);
        return (window_flags & SDL_WINDOW_HIDDEN);
    }

    void Window::MinimiseWindow(bool minimised) {
        SDL_WindowFlags window_flags = SDL_GetWindowFlags(mWindow);

        if (window_flags & SDL_WINDOW_MINIMIZED && minimised == true) {
            CE_LOG(LogLevel::Warn, "[Window] Window is already hidden. It cannot be hidden again");
            return;
        } else if (!(window_flags & SDL_WINDOW_MINIMIZED) && minimised == false) {
            CE_LOG(LogLevel::Warn, "[Window] Window is already minimised. It cannot be minimsed again]");
            return;
        }

        if (minimised) {
            SDL_MinimizeWindow(mWindow);
        } else {
            SDL_RestoreWindow(mWindow);
        }
    }

    bool Window::IsMinimised() {
        SDL_WindowFlags window_flags = SDL_GetWindowFlags(mWindow);
        return (window_flags & SDL_WINDOW_MINIMIZED);
    }

    bool Window::SetWindowSize(WindowSize window_size) {
        if (!SDL_SetWindowSize(mWindow, window_size.w, window_size.h)) {
            CE_LOG(LogLevel::Error, "[Window] Failed to set window size, {}", SDL_GetError());
            return false;
        }

        return true;
    }

    Window::WindowSize Window::GetWindowSize() {
        WindowSize size;
        SDL_GetWindowSizeInPixels(mWindow, &size.w, &size.h);
        return size;
    }

    bool Window::SetWindowTitle(const std::string& title) {
        if (!SDL_SetWindowTitle(mWindow, title.c_str())) {
            CE_LOG(LogLevel::Error, "[Window] Failed to set window title, {}", SDL_GetError());
            return false;
        }

        return true;
    }

    std::string Window::GetWindowTitle() {
        return SDL_GetWindowTitle(mWindow);
    }

    void Window::SetWindowResizable(bool resizable) {
        SDL_WindowFlags window_flags = SDL_GetWindowFlags(mWindow);

        if (window_flags & SDL_WINDOW_RESIZABLE && resizable == true) {
            CE_LOG(LogLevel::Warn, "[Window] Window is already resizable. It cannot be set to resizable again");
            return;
        } else if (!(window_flags & SDL_WINDOW_RESIZABLE) && resizable == false) {
            CE_LOG(LogLevel::Warn, "[Window] Window is already un-resizable. It cannot not be un-resizable again");
            return;
        }

        SDL_SetWindowResizable(mWindow, resizable);
    }

    bool Window::IsWindowResizable() {
        SDL_WindowFlags window_flags = SDL_GetWindowFlags(mWindow);
        return (window_flags & SDL_WINDOW_RESIZABLE);
    }

    bool Window::SetWindowMode(WindowMode mode) {
        switch (mode) {
            case WindowMode::Fullscreen: {
                const SDL_DisplayID display_id = SDL_GetDisplayForWindow(mWindow);
                if (display_id == 0) {
                    CE_LOG(LogLevel::Warn,
                        "[Window] Failed to find display for window, using desktop fullscreen: {}",
                        SDL_GetError());

                    if (!SDL_SetWindowFullscreenMode(mWindow, nullptr) ||
                        !SDL_SetWindowFullscreen(mWindow, true)) {
                        CE_LOG(LogLevel::Error,
                            "[Window] Failed to enter desktop fullscreen: {}",
                            SDL_GetError());
                    }

                    return false;
                }

                const SDL_DisplayMode* desktop_displaymode =
                    SDL_GetDesktopDisplayMode(display_id);

                const WindowSize window_size = GetWindowSize();

                const float refresh_rate =
                    desktop_displaymode != nullptr
                        ? desktop_displaymode->refresh_rate
                        : 0.0f;

                SDL_DisplayMode fullscreen_mode = {};

                const bool found_mode =
                    SDL_GetClosestFullscreenDisplayMode(
                        display_id,
                        window_size.w,
                        window_size.h,
                        refresh_rate,
                        false,
                        &fullscreen_mode);

                if (!found_mode) {
                    CE_LOG(LogLevel::Warn,
                        "[Window] No matching exclusive fullscreen mode for {}x{}, "
                        "using desktop fullscreen: {}",
                        window_size.w,
                        window_size.h,
                        SDL_GetError());

                    if (!SDL_SetWindowFullscreenMode(mWindow, nullptr) ||
                        !SDL_SetWindowFullscreen(mWindow, true)) {
                        CE_LOG(LogLevel::Error,
                            "[Window] Failed to enter desktop fullscreen: {}",
                            SDL_GetError());
                    }

                    return false;
                }

                if (!SDL_SetWindowFullscreenMode(mWindow, &fullscreen_mode)) {
                    CE_LOG(LogLevel::Warn,
                        "[Window] Failed to set fullscreen mode {}x{} @ {} Hz, "
                        "using desktop fullscreen: {}",
                        fullscreen_mode.w,
                        fullscreen_mode.h,
                        fullscreen_mode.refresh_rate,
                        SDL_GetError());

                    if (!SDL_SetWindowFullscreenMode(mWindow, nullptr) ||
                        !SDL_SetWindowFullscreen(mWindow, true)) {
                        CE_LOG(LogLevel::Error,
                            "[Window] Failed to enter desktop fullscreen: {}",
                            SDL_GetError());
                    }

                    return false;
                }

                if (!SDL_SetWindowFullscreen(mWindow, true)) {
                    CE_LOG(LogLevel::Error,
                        "[Window] Failed to enter fullscreen after selecting "
                        "{}x{} @ {} Hz: {}",
                        fullscreen_mode.w,
                        fullscreen_mode.h,
                        fullscreen_mode.refresh_rate,
                        SDL_GetError());

                    return false;
                }

                CE_LOG(LogLevel::Info,
                    "[Window] Fullscreen mode set to {}x{} @ {} Hz",
                    fullscreen_mode.w,
                    fullscreen_mode.h,
                    fullscreen_mode.refresh_rate);

                mWindowMode = WindowMode::Fullscreen;
                break;
            }

            case WindowMode::Borderless: {
                // nullptr means desktop/borderless fullscreen
                if (!SDL_SetWindowFullscreenMode(mWindow, nullptr)) {
                    CE_LOG(LogLevel::Error,
                        "[Window] Failed to set borderless fullscreen mode: {}",
                        SDL_GetError());
                    return false;
                }

                if (!SDL_SetWindowFullscreen(mWindow, true)) {
                    CE_LOG(LogLevel::Error,
                        "[Window] Failed to enter borderless fullscreen: {}",
                        SDL_GetError());
                    return false;
                }

                CE_LOG(LogLevel::Info,
                    "[Window] Borderless fullscreen mode enabled");

                mWindowMode = WindowMode::Borderless;
                break;
            }

            case WindowMode::Windowed: {
                if (!SDL_SetWindowFullscreen(mWindow, false)) {
                    CE_LOG(LogLevel::Error,
                        "[Window] Failed to leave fullscreen mode: {}",
                        SDL_GetError());
                    return false;   
                }

                CE_LOG(LogLevel::Info,
                    "[Window] Windowed mode enabled");

                mWindowMode = WindowMode::Windowed;
                break;
            }
        }
        return true;
    }

    Window::WindowMode Window::GetWindowMode() {
        return mWindowMode;
    }

    bool Window::SetWindowIcon(const std::string& path) {
        uint64_t sz = 0;
        if (!mVFS.GetFileSize(path.c_str(), sz) || sz == 0) {
            CE_LOG(LogLevel::Error, "[Window] VFS could not stat '{}' (missing or empty)", path);
            return false;
        }

        VirtualFile* vf = mVFS.OpenFile(path.c_str());
        if (!vf) {
            CE_LOG(LogLevel::Error, "[Window] VFS could not open '{}'", path);
            return false;
        }

        std::vector<uint8_t> fileBytes((size_t)sz);
        mVFS.ReadFile(vf, fileBytes.data(), fileBytes.size());
        mVFS.CloseFile(vf);

        SDL_IOStream* mem = SDL_IOFromConstMem(fileBytes.data(), fileBytes.size());
        if (!mem) {
            CE_LOG(LogLevel::Error, "[Window] SDL_IOFromConstMem failed: {}", SDL_GetError());
            return false;
        }

        SDL_Surface* surface = IMG_Load_IO(mem, true);
        if (!surface) {
            CE_LOG(LogLevel::Error, "[Window] IMG_Load_IO failed for '{}': {}", path, SDL_GetError());
            return false;
        }

        SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!converted) {
            CE_LOG(LogLevel::Error, "[Window] SDL_ConvertSurface failed: {}", SDL_GetError());
            return false;
        }

        SDL_SetWindowIcon(mWindow, converted);
        SDL_DestroySurface(converted);
        return true;
    }
}