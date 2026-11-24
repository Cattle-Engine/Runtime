#pragma once

#include <filesystem>
#include <unordered_map>
#include <memory>
#include <functional>

#include "ce_editor/ui/i_window.hpp"

namespace CE::Editor::Assets {
    class IAssetEditor : public UI::IWindow {
        public:
            virtual  ~IAssetEditor() = default;

            virtual void Open(std::filesystem::path path) = 0;

            virtual void Close() = 0;
    };

    class AssetEditorManager {
        public:
            using EditorFactory = std::function<std::unique_ptr<IAssetEditor>()>;

            template<typename T>
            void Register(const std::string& extension) {
                mFactories[extension] = []() {
                    return std::make_unique<T>();
                };
            }

            IAssetEditor* Open(const std::filesystem::path& path) {
                auto it = mOpenEditors.find(path);

                if (it != mOpenEditors.end())
                    return it->second.get();

                auto factory_it = mFactories.find(path.extension().string());

                if (factory_it == mFactories.end())
                    return nullptr;

                auto editor = factory_it->second();
                editor->Open(path);

                auto* ptr = editor.get();
                mOpenEditors.emplace(path, std::move(editor));

                return ptr;
            }

            void Close(const std::filesystem::path& path) {
                auto it = mOpenEditors.find(path);

                if (it == mOpenEditors.end())
                    return;

                it->second->Close();
                mOpenEditors.erase(it);
            }

            IAssetEditor* Get(const std::filesystem::path& path) const {
                auto it = mOpenEditors.find(path);

                if (it == mOpenEditors.end())
                    return nullptr;

                return it->second.get();
            }

            bool IsOpen(const std::filesystem::path& path) const {
                return mOpenEditors.contains(path);
            }

        private:
            std::unordered_map<std::string, EditorFactory> mFactories;

            std::unordered_map<std::filesystem::path, std::unique_ptr<IAssetEditor>> mOpenEditors;
    };
}