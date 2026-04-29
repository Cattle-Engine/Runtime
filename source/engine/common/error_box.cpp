#include <cstdlib>
#include <string>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic pop

#include <SDL3/SDL.h>

void ShowError(const std::string& msg) {
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        "Fatal Error!",
        msg.c_str(),
        nullptr
    );
}
