#include "engine/input/input_binder.hpp"

#include "ce_common/tdf.hpp"
#include "engine/common/fs/file_provider.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/common/fs/ifile_stream.hpp"

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

        auto file = mVFS.OpenFile(path);
        if (!file) {
            CE_LOG(LogLevel::Error, "[BindingManager] Failed to load bindings from path {}", path);
            return;
        }

        try {
            Common::FS::VFS::IOStream stream(std::move(file));
            CE::TDF tdf = CE::TDF::Read(stream);

            if (tdf.Type() != CE::TDF::Type::Object) {
                CE_LOG(LogLevel::Error, "[BindingManager] Invalid bindings file: root must be an Object");
                return;
            }

            const auto& object = tdf.Get<CE::TDF::Object>();

            for (const auto& [key, value] : object) {
                if (value.Type() != CE::TDF::Type::Array)
                    continue;

                const auto& array = value.Get<CE::TDF::Array>();

                std::vector<Binding> bindings;
                bindings.reserve(array.size());

                for (const auto& entry : array) {
                    if (entry.Type() != CE::TDF::Type::String)
                        continue;

                    const std::string& str = entry.Get<std::string>();

                    const auto pos = str.find(':');
                    if (pos == std::string::npos)
                        continue;

                    const std::string type = str.substr(0, pos);

                    try {
                        const int code = std::stoi(str.substr(pos + 1));

                        if (type == "Keyboard") {
                            bindings.emplace_back(static_cast<KeyboardKeys>(code));
                        } else if (type == "Mouse") {
                            bindings.emplace_back(static_cast<MouseButtons>(code));
                        }
                    } catch (const std::exception&) {
                        CE_LOG(LogLevel::Warn, "[BindingManager] Invalid binding: {}", str);
                    }
                }

                mBindings[key] = std::move(bindings);
            }
        } catch (const std::exception& e) {
            CE_LOG(LogLevel::Error, "[BindingManager] Failed to parse bindings: {}", e.what());
        }
    }


    void BindingManager::FlushBindings(const std::string& path) {
        CE::TDF tdf;

        for (const auto& [name, bindings] : mBindings) {
            CE::TDF array;

            for (const auto& binding : bindings) {
                array.PushBack(Binding2String(binding));
            }

            tdf[name] = std::move(array);
        }

        auto file = mVFS.OpenFile(path, Common::FS::VFS::OpenFlags::Write);
        if (!file) {
            CE_LOG(LogLevel::Error, "[BindingManager] Failed to save bindings to path {}", path);
            return;
        }

        try {
            Common::FS::VFS::IOStream stream(std::move(file));
            tdf.Write(stream);
        } catch (const std::exception& e) {
            CE_LOG(LogLevel::Error, "[BindingManager] Failed to write bindings: {}", e.what());
        }
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
    BindingManager::BindingManager(Common::FS::VFS::VFS& vfs, Keyboard& keyboard_mgr, Mouse& mouse_mgr)
        : mVFS(vfs), mKeyboardManager(keyboard_mgr), mMouseManager(mouse_mgr) {}
} // namespace CE::Input::Bindings