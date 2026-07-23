#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "engine/common/fs/vfs.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/input/mouse.hpp"

namespace CE::Input::Bindings {
    using Binding = std::variant<KeyboardKeys, MouseButtons>;
    class BindingManager {
      public:
        BindingManager(VFS::VFS& vfs, Keyboard& keyboard_mgr, Mouse& mouse_mgr);
        ~BindingManager();

        bool IsBindingPressed(const std::string& binding_name);
        bool IsBindingDown(const std::string& binding_name);
        bool IsBindingReleased(const std::string& binding_name);

        void AddBinding(const std::string& name, Binding binding);
        void RemoveBinding(const std::string& name, Binding binding);
        void RemoveEntireBinding(const std::string& name);

        void FlushBindings(const std::string& path);
        void LoadBindings(const std::string& path);
        void ResetBindings();

      private:
        const std::vector<Binding>& GetBinding(const std::string& name);
        std::string Binding2String(const Binding& b);
        VFS::VFS& mVFS;
        Keyboard& mKeyboardManager;
        Mouse& mMouseManager;
        std::unordered_map<std::string, std::vector<Binding>> mBindings;
    };
} // namespace CE::Input::Bindings