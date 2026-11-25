#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <magic.h>

#include "ce_common/tracelog.hpp"
#include "ce_editor/ui/i_window.hpp"
#include "ce_runtime/common/fs/vfs.hpp"

namespace CE::Editor {
    class EditorInstance;
}

namespace CE::Editor::Assets {
    class IAssetEditor : public UI::IWindow {
    public:
        using UI::IWindow::IWindow;
        virtual ~IAssetEditor() = default;
        
        virtual void Open(std::string_view vfs_path) = 0;
        virtual void Close() = 0;
    };
    
    class AssetEditorManager {
        public:
            using EditorFactory = std::function<std::unique_ptr<IAssetEditor>()>;
            
            explicit AssetEditorManager(
                EditorInstance& instance,
                Common::FS::VFS::VFS& vfs,
                std::string_view magic_db_vfs_path = {}
            );
            
            ~AssetEditorManager();
            
            AssetEditorManager(const AssetEditorManager&) = delete;
            AssetEditorManager& operator=(const AssetEditorManager&) = delete;
            
            AssetEditorManager(AssetEditorManager&& other) noexcept;
            AssetEditorManager& operator=(AssetEditorManager&& other) = delete;
            
            template<typename T>
            void Register(const std::string& mime_pattern) {
                Register(mime_pattern, [this]() {
                    return std::make_unique<T>(this->mInstance);
                });
            }
            
            void Register(const std::string& mime_pattern, EditorFactory factory);
            
            static bool MatchWildcard(std::string_view pattern, std::string_view value);
            
            std::optional<std::string> DetectMimeType(std::string_view vfs_path) const;
            
            bool CanOpen(std::string_view vfs_path) const;
            IAssetEditor* Open(std::string_view vfs_path);
            
            void Close(std::string_view vfs_path);
            void CloseAll();
            
            IAssetEditor* Get(std::string_view vfs_path) const;
            bool IsOpen(std::string_view vfs_path) const;
            
            bool AddMagicDatabase(std::string_view vfs_path);
        private:
            struct ProviderEntry {
                std::string pattern;
                EditorFactory factory;
            };
            
            struct MatchScore {
                bool is_exact = false;
                size_t literal_count = 0;
                std::ptrdiff_t neg_star_count = 0;
                std::ptrdiff_t neg_question_count = 0;
                size_t index = 0;
                
                bool operator>(const MatchScore& other) const;
            };
            
            static char ToLowerAscii(char c);
            
            static MatchScore ScorePattern(
                std::string_view pattern,
                size_t index
            );
            
            const EditorFactory* FindFactory(std::string_view mime_type) const;
            
            magic_t mMagic = nullptr;
            EditorInstance& mInstance;
            Common::FS::VFS::VFS& mVFS;
            std::vector<std::string> mMagicDatabaseContents;
            std::vector<ProviderEntry> mProviders;
            std::unordered_map<std::string, std::unique_ptr<IAssetEditor>> mOpenEditors;
    };
}
