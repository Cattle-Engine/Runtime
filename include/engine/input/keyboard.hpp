#pragma once

#include <SDL3/SDL.h>
#include <string>

namespace CE::Input {
    // "Borrowed" from raylib, and then modified
    // Probably should put the licence for raylib at the bottom, so... look at the bottom
    enum class KeyboardKeys {
        // Alphanumeric keys
        KEY_APOSTROPHE      = SDL_SCANCODE_APOSTROPHE,       // Key: '
        KEY_COMMA           = SDL_SCANCODE_COMMA,       // Key: ,
        KEY_MINUS           = SDL_SCANCODE_MINUS,       // Key: -
        KEY_PERIOD          = SDL_SCANCODE_PERIOD,       // Key: .
        KEY_SLASH           = SDL_SCANCODE_SLASH,       // Key: /
        KEY_ZERO            = SDL_SCANCODE_0,       // Key: 0
        KEY_ONE             = SDL_SCANCODE_1,       // Key: 1
        KEY_TWO             = SDL_SCANCODE_2,       // Key: 2
        KEY_THREE           = SDL_SCANCODE_3,       // Key: 3
        KEY_FOUR            = SDL_SCANCODE_4,       // Key: 4
        KEY_FIVE            = SDL_SCANCODE_5,       // Key: 5
        KEY_SIX             = SDL_SCANCODE_6,       // Key: 6
        KEY_SEVEN           = SDL_SCANCODE_7,       // Key: 7
        KEY_EIGHT           = SDL_SCANCODE_8,       // Key: 8
        KEY_NINE            = SDL_SCANCODE_9,       // Key: 9
        KEY_SEMICOLON       = SDL_SCANCODE_SEMICOLON,       // Key: ;
        KEY_EQUAL           = SDL_SCANCODE_EQUALS,       // Key: =
        KEY_A               = SDL_SCANCODE_A,       // Key: A | a
        KEY_B               = SDL_SCANCODE_B,       // Key: B | b
        KEY_C               = SDL_SCANCODE_C,       // Key: C | c
        KEY_D               = SDL_SCANCODE_D,       // Key: D | d
        KEY_E               = SDL_SCANCODE_E,       // Key: E | e
        KEY_F               = SDL_SCANCODE_F,       // Key: F | f
        KEY_G               = SDL_SCANCODE_G,       // Key: G | g
        KEY_H               = SDL_SCANCODE_H,       // Key: H | h
        KEY_I               = SDL_SCANCODE_I,       // Key: I | i
        KEY_J               = SDL_SCANCODE_J,       // Key: J | j
        KEY_K               = SDL_SCANCODE_K,       // Key: K | k
        KEY_L               = SDL_SCANCODE_L,       // Key: L | l
        KEY_M               = SDL_SCANCODE_M,       // Key: M | m
        KEY_N               = SDL_SCANCODE_N,       // Key: N | n
        KEY_O               = SDL_SCANCODE_O,       // Key: O | o
        KEY_P               = SDL_SCANCODE_P,       // Key: P | p
        KEY_Q               = SDL_SCANCODE_Q,       // Key: Q | q
        KEY_R               = SDL_SCANCODE_R,       // Key: R | r
        KEY_S               = SDL_SCANCODE_S,       // Key: S | s
        KEY_T               = SDL_SCANCODE_T,       // Key: T | t
        KEY_U               = SDL_SCANCODE_U,       // Key: U | u
        KEY_V               = SDL_SCANCODE_V,       // Key: V | v
        KEY_W               = SDL_SCANCODE_W,       // Key: W | w
        KEY_X               = SDL_SCANCODE_X,       // Key: X | x
        KEY_Y               = SDL_SCANCODE_Y,       // Key: Y | y
        KEY_Z               = SDL_SCANCODE_Z,       // Key: Z | z
        KEY_LEFT_BRACKET    = SDL_SCANCODE_LEFTBRACKET,       // Key: [
        KEY_BACKSLASH       = SDL_SCANCODE_BACKSLASH,       // Key: '\'
        KEY_RIGHT_BRACKET   = SDL_SCANCODE_RIGHTBRACKET,       // Key: ]
        KEY_GRAVE           = SDL_SCANCODE_GRAVE,       // Key: `
        // Function keys
        KEY_SPACE           = SDL_SCANCODE_SPACE,       // Key: Space
        KEY_ESCAPE          = SDL_SCANCODE_ESCAPE,      // Key: Esc
        KEY_ENTER           = SDL_SCANCODE_RETURN,      // Key: Enter
        KEY_TAB             = SDL_SCANCODE_TAB,      // Key: Tab
        KEY_BACKSPACE       = SDL_SCANCODE_BACKSPACE,      // Key: Backspace
        KEY_INSERT          = SDL_SCANCODE_INSERT,      // Key: Ins
        KEY_DELETE          = SDL_SCANCODE_DELETE,      // Key: Del
        KEY_RIGHT           = SDL_SCANCODE_RIGHT,      // Key: Cursor right
        KEY_LEFT            = SDL_SCANCODE_LEFT,      // Key: Cursor left
        KEY_DOWN            = SDL_SCANCODE_DOWN,      // Key: Cursor down
        KEY_UP              = SDL_SCANCODE_UP,      // Key: Cursor up
        KEY_PAGE_UP         = SDL_SCANCODE_PAGEUP,      // Key: Page up
        KEY_PAGE_DOWN       = SDL_SCANCODE_PAGEDOWN,      // Key: Page down
        KEY_HOME            = SDL_SCANCODE_HOME,      // Key: Home
        KEY_END             = SDL_SCANCODE_END,      // Key: End
        KEY_CAPS_LOCK       = SDL_SCANCODE_CAPSLOCK,      // Key: Caps lock
        KEY_SCROLL_LOCK     = SDL_SCANCODE_SCROLLLOCK,      // Key: Scroll down
        KEY_NUM_LOCK        = SDL_SCANCODE_NUMLOCKCLEAR,      // Key: Num lock
        KEY_PRINT_SCREEN    = SDL_SCANCODE_PRINTSCREEN,      // Key: Print screen
        KEY_PAUSE           = SDL_SCANCODE_PAUSE,      // Key: Pause
        KEY_F1              = SDL_SCANCODE_F1,      // Key: F1
        KEY_F2              = SDL_SCANCODE_F2,      // Key: F2
        KEY_F3              = SDL_SCANCODE_F3,      // Key: F3
        KEY_F4              = SDL_SCANCODE_F4,      // Key: F4
        KEY_F5              = SDL_SCANCODE_F5,      // Key: F5
        KEY_F6              = SDL_SCANCODE_F6,      // Key: F6
        KEY_F7              = SDL_SCANCODE_F7,      // Key: F7
        KEY_F8              = SDL_SCANCODE_F8,      // Key: F8
        KEY_F9              = SDL_SCANCODE_F9,      // Key: F9
        KEY_F10             = SDL_SCANCODE_F10,      // Key: F10
        KEY_F11             = SDL_SCANCODE_F11,      // Key: F11
        KEY_F12             = SDL_SCANCODE_F12,      // Key: F12
        KEY_LEFT_SHIFT      = SDL_SCANCODE_LSHIFT,      // Key: Shift left
        KEY_LEFT_CONTROL    = SDL_SCANCODE_LCTRL,      // Key: Control left
        KEY_LEFT_ALT        = SDL_SCANCODE_LALT,      // Key: Alt left
        KEY_LEFT_SUPER      = SDL_SCANCODE_LGUI,      // Key: Super left
        KEY_RIGHT_SHIFT     = SDL_SCANCODE_RSHIFT,      // Key: Shift right
        KEY_RIGHT_CONTROL   = SDL_SCANCODE_RCTRL,      // Key: Control right
        KEY_RIGHT_ALT       = SDL_SCANCODE_RALT,      // Key: Alt right
        KEY_RIGHT_SUPER     = SDL_SCANCODE_RGUI,      // Key: Super right
        KEY_KB_MENU         = SDL_SCANCODE_APPLICATION,      // Key: KB menu
        // Keypad keys
        KEY_KP_0            = SDL_SCANCODE_KP_0,      // Key: Keypad 0
        KEY_KP_1            = SDL_SCANCODE_KP_1,      // Key: Keypad 1
        KEY_KP_2            = SDL_SCANCODE_KP_2,      // Key: Keypad 2
        KEY_KP_3            = SDL_SCANCODE_KP_3,      // Key: Keypad 3
        KEY_KP_4            = SDL_SCANCODE_KP_4,      // Key: Keypad 4
        KEY_KP_5            = SDL_SCANCODE_KP_5,      // Key: Keypad 5
        KEY_KP_6            = SDL_SCANCODE_KP_6,      // Key: Keypad 6
        KEY_KP_7            = SDL_SCANCODE_KP_7,      // Key: Keypad 7
        KEY_KP_8            = SDL_SCANCODE_KP_8,      // Key: Keypad 8
        KEY_KP_9            = SDL_SCANCODE_KP_9,      // Key: Keypad 9
        KEY_KP_DECIMAL      = SDL_SCANCODE_KP_DECIMAL,      // Key: Keypad .
        KEY_KP_DIVIDE       = SDL_SCANCODE_KP_DIVIDE,      // Key: Keypad /
        KEY_KP_MULTIPLY     = SDL_SCANCODE_KP_MULTIPLY,      // Key: Keypad *
        KEY_KP_SUBTRACT     = SDL_SCANCODE_KP_MINUS,      // Key: Keypad -
        KEY_KP_ADD          = SDL_SCANCODE_KP_PLUS,      // Key: Keypad +
        KEY_KP_ENTER        = SDL_SCANCODE_KP_ENTER,      // Key: Keypad Enter
        KEY_KP_EQUAL        = SDL_SCANCODE_KP_EQUALS,      // Key: Keypad =
    };
    
    class Keyboard {
        public:
            Keyboard(int windowID);
            void Update();
            bool IsKeyReleased(KeyboardKeys key) const;
            bool IsKeyPressed(KeyboardKeys key) const;
            bool IsKeyDown(KeyboardKeys key) const;
            std::string GetPressedKeysString() const;

        private:
            bool gCurrent[SDL_SCANCODE_COUNT] = {};
            bool gPrevious[SDL_SCANCODE_COUNT] = {};
            int gWindowID;
    };
}

/*
Copyright (c) 2013-2026 Ramon Santamaria (@raysan5)

This software is provided "as-is", without any express or implied warranty. In no event 
will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial 
applications, and to alter it and redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim that you 
  wrote the original software. If you use this software in a product, an acknowledgment 
  in the product documentation would be appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not be misrepresented
  as being the original software.

  3. This notice may not be removed or altered from any source distribution.*/
