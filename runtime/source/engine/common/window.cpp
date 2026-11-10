#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "engine/common/window.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Common {
    Window::Window(Core::GameState::GameStateManager& game_state, Common::FS::VFS::VFS& vfs, const std::string& window_title, WindowSize size, SDL_WindowFlags flags): mVFS(vfs), mGameState(game_state) {
        mWindow = SDL_CreateWindow(window_title.c_str(), size.w, size.h, flags);
        if (!mWindow) {
            CE_LOG(LogLevel::Error, "[Window] Failed to create window: {}", SDL_GetError());
            throw std::runtime_error("Failed to create window");
        }

        mWindowedSize = size;
        mFullscreenSize = size;
        mWindowMode = WindowMode::Windowed;
    }

    Window::~Window() {
        SDL_DestroyWindow(mWindow);
    }

    SDL_Window* Window::GetWindow() {
        return mWindow;
    }

    void Window::HideWindow(bool hidden) {
        SDL_WindowFlags window_flags = SDL_GetWindowFlags(mWindow);

        if ((window_flags & SDL_WINDOW_HIDDEN) && hidden) {
            CE_LOG(LogLevel::Warn, "[Window] Window is already hidden. It cannot be hidden again!");
            return;
        } else if (!(window_flags & SDL_WINDOW_HIDDEN) && !hidden) {
            CE_LOG(LogLevel::Warn, "[Window] Window is already visible. It cannot be unhidden");
            return;
        }

        if (hidden) {
            SDL_HideWindow(mWindow);
        } else {
            SDL_ShowWindow(mWindow);
        }
    }

    bool Window::IsHidden() {
        SDL_WindowFlags window_flags = SDL_GetWindowFlags(mWindow);
        return (window_flags & SDL_WINDOW_HIDDEN) != 0;
    }

    void Window::MinimiseWindow(bool minimised) {
        SDL_WindowFlags window_flags = SDL_GetWindowFlags(mWindow);

        if ((window_flags & SDL_WINDOW_MINIMIZED) && minimised) {
            CE_LOG(LogLevel::Warn, "[Window] Window is already minimised. It cannot be minimised again");
            return;
        } else if (!(window_flags & SDL_WINDOW_MINIMIZED) && !minimised) {
            CE_LOG(LogLevel::Warn, "[Window] Window is not minimised. It cannot be restored");
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
        return (window_flags & SDL_WINDOW_MINIMIZED) != 0;
    }

    bool Window::SetWindowSize(WindowSize window_size, WindowMode mode) {
        if (mode == WindowMode::Borderless) {
            CE_LOG(LogLevel::Error, "[Window] Borderless window does not have a configurable size");
            return false;
        }

        // Always remember the size for the requested mode
        if (mode == WindowMode::Fullscreen) {
            mFullscreenSize = window_size;
        } else {
            mWindowedSize = window_size;
        }

        // Only touch the real window if that is the mode we are currently in
        if (mode == mWindowMode) {
            if (mode == WindowMode::Fullscreen) {
                // Exclusive fullscreen: re-pick the display mode instead of resizing
                SetWindowMode(WindowMode::Fullscreen);
            } else if (!SDL_SetWindowSize(mWindow, window_size.w, window_size.h)) {
                CE_LOG(LogLevel::Error, "[Window] Failed to set window size, {}", SDL_GetError());
                return false;
            }
        }

        mGameState.Emit(mode == WindowMode::Fullscreen
            ? "CE_WINDOW_SIZE_CHANGE_FULLSCREEN"
            : "CE_WINDOW_SIZE_CHANGE_WINDOWED");

        return true;
    }

    Window::WindowSize Window::GetWindowSize(WindowMode mode) {
        if (mode == WindowMode::Windowed) {
            return mWindowedSize;
        } else if (mode == WindowMode::Fullscreen) {
            return mFullscreenSize;
        } else {
            // Borderless uses the desktop resolution of the display the window is on
            const SDL_DisplayID display_id = SDL_GetDisplayForWindow(mWindow);
            if (display_id != 0) {
                const SDL_DisplayMode* desktop = SDL_GetDesktopDisplayMode(display_id);
                if (desktop) {
                    return {desktop->w, desktop->h};
                }
            }

            CE_LOG(LogLevel::Error, "[Window] Failed to determine borderless window size: {}", SDL_GetError());
            return {0, 0};
        }
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

        if ((window_flags & SDL_WINDOW_RESIZABLE) && resizable) {
            CE_LOG(LogLevel::Warn, "[Window] Window is already resizable. It cannot be set to resizable again");
            return;
        } else if (!(window_flags & SDL_WINDOW_RESIZABLE) && !resizable) {
            CE_LOG(LogLevel::Warn, "[Window] Window is already un-resizable. It cannot be un-resizable again");
            return;
        }

        SDL_SetWindowResizable(mWindow, resizable);
    }

    bool Window::IsWindowResizable() {
        SDL_WindowFlags window_flags = SDL_GetWindowFlags(mWindow);
        return (window_flags & SDL_WINDOW_RESIZABLE) != 0;
    }

    // mode == nullptr -> borderless / desktop fullscreen
    // mode != nullptr -> exclusive fullscreen with that display mode
    bool Window::ApplyFullscreen(const SDL_DisplayMode* mode) {
        if (!SDL_SetWindowFullscreenMode(mWindow, mode)) {
            CE_LOG(LogLevel::Error, "[Window] SDL_SetWindowFullscreenMode failed: {}", SDL_GetError());
            return false;
        }

        if (!SDL_SetWindowFullscreen(mWindow, true)) {
            CE_LOG(LogLevel::Error, "[Window] SDL_SetWindowFullscreen failed: {}", SDL_GetError());
            return false;
        }

        // Fullscreen transitions are asynchronous on many platforms; wait for completion
        if (!SDL_SyncWindow(mWindow)) {
            CE_LOG(LogLevel::Warn, "[Window] Fullscreen transition did not finish in time: {}", SDL_GetError());
        }

        return true;
    }

    bool Window::SetWindowMode(WindowMode mode) {
        switch (mode) {
            case WindowMode::Fullscreen: {
                SDL_DisplayMode fullscreen_mode = {};
                const SDL_DisplayMode* chosen = nullptr;

                const SDL_DisplayID display_id = SDL_GetDisplayForWindow(mWindow);
                if (display_id != 0) {
                    const SDL_DisplayMode* desktop_displaymode = SDL_GetDesktopDisplayMode(display_id);
                    const float refresh_rate = desktop_displaymode ? desktop_displaymode->refresh_rate : 0.0f;

                    if (SDL_GetClosestFullscreenDisplayMode(
                            display_id,
                            mFullscreenSize.w,
                            mFullscreenSize.h,
                            refresh_rate,
                            false,
                            &fullscreen_mode)) {
                        chosen = &fullscreen_mode;
                    } else {
                        CE_LOG(LogLevel::Warn,
                            "[Window] No matching exclusive fullscreen mode for {}x{}: {}",
                            mFullscreenSize.w,
                            mFullscreenSize.h,
                            SDL_GetError());
                    }
                } else {
                    CE_LOG(LogLevel::Warn,
                        "[Window] Failed to find display for window: {}",
                        SDL_GetError());
                }

                if (chosen && ApplyFullscreen(chosen)) {
                    CE_LOG(LogLevel::Info,
                        "[Window] Fullscreen mode set to {}x{} @ {} Hz",
                        fullscreen_mode.w,
                        fullscreen_mode.h,
                        fullscreen_mode.refresh_rate);

                    mWindowMode = WindowMode::Fullscreen;
                    mGameState.Emit("CE_WINDOW_MODE_CHANGED_FULLSCREEN");
                    return true;
                }

                // Fallback to desktop fullscreen, and keep the tracked state honest
                CE_LOG(LogLevel::Warn, "[Window] Falling back to desktop (borderless) fullscreen");

                if (!ApplyFullscreen(nullptr)) {
                    return false;
                }

                mWindowMode = WindowMode::Borderless;
                mGameState.Emit("CE_WINDOW_MODE_CHANGED_BORDERLESS");
                return false; // the requested mode was not honoured
            }

            case WindowMode::Borderless: {
                if (!ApplyFullscreen(nullptr)) {
                    return false;
                }

                CE_LOG(LogLevel::Info, "[Window] Borderless fullscreen mode enabled");

                mWindowMode = WindowMode::Borderless;
                mGameState.Emit("CE_WINDOW_MODE_CHANGED_BORDERLESS");
                return true;
            }

            case WindowMode::Windowed: {
                if (!SDL_SetWindowFullscreen(mWindow, false)) {
                    CE_LOG(LogLevel::Error,
                        "[Window] Failed to leave fullscreen mode: {}",
                        SDL_GetError());
                    return false;
                }

                // Wait for the transition before resizing, otherwise the size gets ignored/overwritten
                SDL_SyncWindow(mWindow);

                if (!SDL_SetWindowSize(mWindow, mWindowedSize.w, mWindowedSize.h)) {
                    CE_LOG(LogLevel::Warn, "[Window] Failed to restore windowed size: {}", SDL_GetError());
                }
                SDL_SyncWindow(mWindow);

                CE_LOG(LogLevel::Info, "[Window] Windowed mode enabled");

                mWindowMode = WindowMode::Windowed;
                mGameState.Emit("CE_WINDOW_MODE_CHANGED_WINDOWED");
                return true;
            }
        }

        return false;
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

        auto vf = mVFS.OpenFile(path);
        if (!vf) {
            CE_LOG(LogLevel::Error, "[Window] VFS could not open '{}'", path);
            return false;
        }

        std::vector<uint8_t> fileBytes((size_t)sz);
        if (!vf->Read(fileBytes.data(), fileBytes.size())) {
            CE_LOG(LogLevel::Error, "[Window] VFS could not read '{}'", path);
            return false;
        }

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