#include "engine/scripting/bindings/input/keyboard_enum_registerer.hpp"
#include "engine/input/keyboard.hpp"

namespace CE::Scripting::Bindings{
    bool RegisterKeyboardEnum(asIScriptEngine* engine) {
        int result = engine->RegisterEnum("KeyboardKeys");
        if (result < 0) {
            return false;
        }

    #define CE_REGISTER_KEY(name)                                                                                          \
        result = engine->RegisterEnumValue("KeyboardKeys", #name, static_cast<int>(CE::Input::KeyboardKeys::name));        \
        if (result < 0) {                                                                                                  \
            return false;                                                                                                  \
        }

            CE_REGISTER_KEY(KEY_APOSTROPHE);
            CE_REGISTER_KEY(KEY_COMMA);
            CE_REGISTER_KEY(KEY_MINUS);
            CE_REGISTER_KEY(KEY_PERIOD);
            CE_REGISTER_KEY(KEY_SLASH);
            CE_REGISTER_KEY(KEY_ZERO);
            CE_REGISTER_KEY(KEY_ONE);
            CE_REGISTER_KEY(KEY_TWO);
            CE_REGISTER_KEY(KEY_THREE);
            CE_REGISTER_KEY(KEY_FOUR);
            CE_REGISTER_KEY(KEY_FIVE);
            CE_REGISTER_KEY(KEY_SIX);
            CE_REGISTER_KEY(KEY_SEVEN);
            CE_REGISTER_KEY(KEY_EIGHT);
            CE_REGISTER_KEY(KEY_NINE);
            CE_REGISTER_KEY(KEY_SEMICOLON);
            CE_REGISTER_KEY(KEY_EQUAL);
            CE_REGISTER_KEY(KEY_A);
            CE_REGISTER_KEY(KEY_B);
            CE_REGISTER_KEY(KEY_C);
            CE_REGISTER_KEY(KEY_D);
            CE_REGISTER_KEY(KEY_E);
            CE_REGISTER_KEY(KEY_F);
            CE_REGISTER_KEY(KEY_G);
            CE_REGISTER_KEY(KEY_H);
            CE_REGISTER_KEY(KEY_I);
            CE_REGISTER_KEY(KEY_J);
            CE_REGISTER_KEY(KEY_K);
            CE_REGISTER_KEY(KEY_L);
            CE_REGISTER_KEY(KEY_M);
            CE_REGISTER_KEY(KEY_N);
            CE_REGISTER_KEY(KEY_O);
            CE_REGISTER_KEY(KEY_P);
            CE_REGISTER_KEY(KEY_Q);
            CE_REGISTER_KEY(KEY_R);
            CE_REGISTER_KEY(KEY_S);
            CE_REGISTER_KEY(KEY_T);
            CE_REGISTER_KEY(KEY_U);
            CE_REGISTER_KEY(KEY_V);
            CE_REGISTER_KEY(KEY_W);
            CE_REGISTER_KEY(KEY_X);
            CE_REGISTER_KEY(KEY_Y);
            CE_REGISTER_KEY(KEY_Z);
            CE_REGISTER_KEY(KEY_LEFT_BRACKET);
            CE_REGISTER_KEY(KEY_BACKSLASH);
            CE_REGISTER_KEY(KEY_RIGHT_BRACKET);
            CE_REGISTER_KEY(KEY_GRAVE);
            CE_REGISTER_KEY(KEY_SPACE);
            CE_REGISTER_KEY(KEY_ESCAPE);
            CE_REGISTER_KEY(KEY_ENTER);
            CE_REGISTER_KEY(KEY_TAB);
            CE_REGISTER_KEY(KEY_BACKSPACE);
            CE_REGISTER_KEY(KEY_INSERT);
            CE_REGISTER_KEY(KEY_DELETE);
            CE_REGISTER_KEY(KEY_RIGHT);
            CE_REGISTER_KEY(KEY_LEFT);
            CE_REGISTER_KEY(KEY_DOWN);
            CE_REGISTER_KEY(KEY_UP);
            CE_REGISTER_KEY(KEY_PAGE_UP);
            CE_REGISTER_KEY(KEY_PAGE_DOWN);
            CE_REGISTER_KEY(KEY_HOME);
            CE_REGISTER_KEY(KEY_END);
            CE_REGISTER_KEY(KEY_CAPS_LOCK);
            CE_REGISTER_KEY(KEY_SCROLL_LOCK);
            CE_REGISTER_KEY(KEY_NUM_LOCK);
            CE_REGISTER_KEY(KEY_PRINT_SCREEN);
            CE_REGISTER_KEY(KEY_PAUSE);
            CE_REGISTER_KEY(KEY_F1);
            CE_REGISTER_KEY(KEY_F2);
            CE_REGISTER_KEY(KEY_F3);
            CE_REGISTER_KEY(KEY_F4);
            CE_REGISTER_KEY(KEY_F5);
            CE_REGISTER_KEY(KEY_F6);
            CE_REGISTER_KEY(KEY_F7);
            CE_REGISTER_KEY(KEY_F8);
            CE_REGISTER_KEY(KEY_F9);
            CE_REGISTER_KEY(KEY_F10);
            CE_REGISTER_KEY(KEY_F11);
            CE_REGISTER_KEY(KEY_F12);
            CE_REGISTER_KEY(KEY_LEFT_SHIFT);
            CE_REGISTER_KEY(KEY_LEFT_CONTROL);
            CE_REGISTER_KEY(KEY_LEFT_ALT);
            CE_REGISTER_KEY(KEY_LEFT_SUPER);
            CE_REGISTER_KEY(KEY_RIGHT_SHIFT);
            CE_REGISTER_KEY(KEY_RIGHT_CONTROL);
            CE_REGISTER_KEY(KEY_RIGHT_ALT);
            CE_REGISTER_KEY(KEY_RIGHT_SUPER);
            CE_REGISTER_KEY(KEY_KB_MENU);
            CE_REGISTER_KEY(KEY_KP_0);
            CE_REGISTER_KEY(KEY_KP_1);
            CE_REGISTER_KEY(KEY_KP_2);
            CE_REGISTER_KEY(KEY_KP_3);
            CE_REGISTER_KEY(KEY_KP_4);
            CE_REGISTER_KEY(KEY_KP_5);
            CE_REGISTER_KEY(KEY_KP_6);
            CE_REGISTER_KEY(KEY_KP_7);
            CE_REGISTER_KEY(KEY_KP_8);
            CE_REGISTER_KEY(KEY_KP_9);
            CE_REGISTER_KEY(KEY_KP_DECIMAL);
            CE_REGISTER_KEY(KEY_KP_DIVIDE);
            CE_REGISTER_KEY(KEY_KP_MULTIPLY);
            CE_REGISTER_KEY(KEY_KP_SUBTRACT);
            CE_REGISTER_KEY(KEY_KP_ADD);
            CE_REGISTER_KEY(KEY_KP_ENTER);
            CE_REGISTER_KEY(KEY_KP_EQUAL);

    #undef CE_REGISTER_KEY
            return true;
    }
}