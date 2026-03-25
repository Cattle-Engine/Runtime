#include <cstdlib>
#include <string>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic pop

extern "C" {
#include "third_party/tiny_file_dialogs/tinyfiledialogs.h"
}

void ShowError(const std::string& msg) {
    std::string safe_msg;
    safe_msg.reserve(msg.size());
    for (char c : msg) {
        if (c == '"' || c == '\'') safe_msg += '`'; // replace both single and double quotes
        else if (c == '\n' || c == '\r') safe_msg += ' '; // replace newlines
        else safe_msg += c;
    }

    tinyfd_messageBox(
        "Fatal error was caught!",
        safe_msg.c_str(),
        "ok",
        "error",
        1
    );
}
