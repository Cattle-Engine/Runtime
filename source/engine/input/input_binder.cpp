#include "engine/input/input_binder.hpp"

#include "engine/common/fs/tdf.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Input::Bindings {
    const std::vector<Binding>& BindingManager::GetBinding(const std::string& name) {
        static std::vector<Binding> empty;

        auto it = mBindings.find(name);
        if (it == mBindings.end())
            return empty;

        return it->second;
    }

    std::string BindingManager::Binding2String(const Binding& b) {
        return std::visit(
            [](auto&& key) -> std::string {
                using T = std::decay_t<decltype(key)>;

                if constexpr (std::is_same_v<T, KeyboardKeys>)
                    return "Keyboard:" + std::to_string(static_cast<int>(key));
                else if constexpr (std::is_same_v<T, MouseButtons>)
                    return "Mouse:" + std::to_string(static_cast<int>(key));

                return {};
            },
            b);
    }

    void BindingManager::LoadBindings(const std::string& path) {
        mBindings.clear();

        CE::TDF::File file;
        file.load(mVFS, path);

        for (const auto& [key, value] : file.entries) {

            if (value.type != CE::TDF::Type::ArrString) {
                continue;
            }

            std::vector<std::string> raw = CE::TDF::File::readStringArray(value);
            std::vector<Binding> bindings;
            bindings.reserve(raw.size());

            for (const auto& str : raw) {

                auto pos = str.find(':');
                if (pos == std::string::npos) {
                    continue;
                }

                std::string type = str.substr(0, pos);
                int code = std::stoi(str.substr(pos + 1));

                if (type == "Keyboard") {
                    bindings.emplace_back(static_cast<KeyboardKeys>(code));
                } else if (type == "Mouse") {
                    bindings.emplace_back(static_cast<MouseButtons>(code));
                }
            }
            mBindings[key] = std::move(bindings);
        }
    }

    void BindingManager::FlushBindings(const std::string& path) {
        CE::TDF::File file;

        for (const auto& [name, bindings] : mBindings) {
            std::vector<std::string> encoded;
            encoded.reserve(bindings.size());
            for (const auto& b : bindings) {
                encoded.push_back(Binding2String(b));
            }

            file.set(name, CE::TDF::File::makeStringArray(encoded));
        }

        file.save(mVFS, path, 1);
    }

    void BindingManager::AddBinding(const std::string& name, Binding binding) {
        auto& list = mBindings[name];

        for (const auto& b : list) {
            if (b == binding) {
                return;
            }
        }

        list.push_back(std::move(binding));
    }

    void BindingManager::RemoveBinding(const std::string& name, Binding binding) {
        auto it = mBindings.find(name);

        if (it == mBindings.end()) {
            return;
        }

        auto& list = it->second;

        std::erase_if(list, [&](const Binding& b) { return b == binding; });

        if (list.empty()) {
            mBindings.erase(it);
        }
    }

    bool BindingManager::IsBindingPressed(const std::string& binding_name) {
        auto it = mBindings.find(binding_name);

        if (it == mBindings.end()) {
            return false;
        }

        const auto& list = it->second;

        for (const auto& binding : list) {
            bool pressed = std::visit(
                [this](auto&& key) -> bool {
                    using T = std::decay_t<decltype(key)>;

                    if constexpr (std::is_same_v<T, KeyboardKeys>) {
                        return mKeyboardManager.IsKeyPressed(key);
                    } else if constexpr (std::is_same_v<T, MouseButtons>) {
                        return mMouseManager.IsButtonPressed(key);
                    }

                    return false;
                },
                binding);

            if (pressed) {
                return true;
            }
        }
        return false;
    }

    bool BindingManager::IsBindingDown(const std::string& binding_name) {
        auto it = mBindings.find(binding_name);

        if (it == mBindings.end()) {
            return false;
        }

        const auto& list = it->second;

        for (const auto& binding : list) {
            bool pressed = std::visit(
                [this](auto&& key) -> bool {
                    using T = std::decay_t<decltype(key)>;

                    if constexpr (std::is_same_v<T, KeyboardKeys>) {
                        return mKeyboardManager.IsKeyDown(key);
                    } else if constexpr (std::is_same_v<T, MouseButtons>) {
                        return mMouseManager.IsButtonDown(key);
                    }

                    return false;
                },
                binding);

            if (pressed) {
                return true;
            }
        }
        return false;
    }

    bool BindingManager::IsBindingReleased(const std::string& binding_name) {
        auto it = mBindings.find(binding_name);

        if (it == mBindings.end()) {
            return false;
        }

        const auto& list = it->second;

        for (const auto& binding : list) {
            bool pressed = std::visit(
                [this](auto&& key) -> bool {
                    using T = std::decay_t<decltype(key)>;

                    if constexpr (std::is_same_v<T, KeyboardKeys>) {
                        return mKeyboardManager.IsKeyReleased(key);
                    } else if constexpr (std::is_same_v<T, MouseButtons>) {
                        return mMouseManager.IsButtonReleased(key);
                    }

                    return false;
                },
                binding);

            if (pressed) {
                return true;
            }
        }
        return false;
    }

    void BindingManager::RemoveEntireBinding(const std::string& name) {
        mBindings.erase(name);
    }
    void BindingManager::ResetBindings() {
        mBindings.clear();
    }
    BindingManager::BindingManager(VFS::VFS& vfs, Keyboard& keyboard_mgr, Mouse& mouse_mgr)
        : mVFS(vfs), mKeyboardManager(keyboard_mgr), mMouseManager(mouse_mgr) {}
} // namespace CE::Input::Bindings