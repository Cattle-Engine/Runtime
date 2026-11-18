#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ce_runtime/common/fs/vfs.hpp"
#include "ce_runtime/input/keyboard.hpp"
#include "ce_runtime/input/mouse.hpp"

namespace CE::Input::Bindings {
    using Binding = std::variant<KeyboardKeys, MouseButtons>;
    class BindingManager {
      public:
        BindingManager(Common::FS::VFS::VFS& vfs, Keyboard& keyboard_mgr, Mouse& mouse_mgr);
        ~BindingManager() = default;

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
        Common::FS::VFS::VFS& mVFS;
        Keyboard& mKeyboardManager;
        Mouse& mMouseManager;
        std::unordered_map<std::string, std::vector<Binding>> mBindings;
    };
} // namespace CE::Input::Bindings
