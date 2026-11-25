#include "ce_editor/assets/asset_editor_manager.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <utility>

namespace CE::Editor::Assets {
     AssetEditorManager::AssetEditorManager(
        EditorInstance& instance,
        Common::FS::VFS::VFS& vfs,
        std::string_view magic_db_vfs_path
    ) : mInstance(instance), mVFS(vfs) {
        mMagic = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);
        
        if (!mMagic) {
            CE_LOG(
                CECommon::LogLevel::Error,
                "[AssetEditorManager] Failed to initialize libmagic"
            );
            return;
        }
        
        if (!AddMagicDatabase(magic_db_vfs_path)) {
            if (magic_load(mMagic, nullptr) != 0) {
                const char* err = magic_error(mMagic);
                
                CE_LOG(
                    CECommon::LogLevel::Error,
                    "[AssetEditorManager] Failed to load default magic database: {}",
                    err ? err : "unknown error"
                );
                
                magic_close(mMagic);
                mMagic = nullptr;
            }
        }
    }
    
    AssetEditorManager::~AssetEditorManager() {
        CloseAll();
        
        if (mMagic) {
            magic_close(mMagic);
            mMagic = nullptr;
        }
    }
    
    AssetEditorManager::AssetEditorManager(AssetEditorManager&& other) noexcept
    : mMagic(std::exchange(other.mMagic, nullptr))
    , mInstance(other.mInstance)
    , mVFS(other.mVFS)
    , mMagicDatabaseContents(std::move(other.mMagicDatabaseContents))
    , mProviders(std::move(other.mProviders))
    , mOpenEditors(std::move(other.mOpenEditors)) {}
    
    void AssetEditorManager::Register(
        const std::string& mime_pattern,
        EditorFactory factory
    ) {
        if (mime_pattern.empty() || !factory) {
            return;
        }
        
        for (auto& provider : mProviders) {
            if (provider.pattern == mime_pattern) {
                provider.factory = std::move(factory);
                return;
            }
        }
        
        mProviders.push_back(ProviderEntry{
            .pattern = mime_pattern,
            .factory = std::move(factory),
        });
    }
    
    bool AssetEditorManager::MatchWildcard(
        std::string_view pattern,
        std::string_view value
    ) {
        size_t p = 0;
        size_t v = 0;
        size_t star_p = std::string_view::npos;
        size_t star_v = 0;
        
        while (v < value.size()) {
            if (p < pattern.size() &&
                (pattern[p] == '?' || ToLowerAscii(pattern[p]) == ToLowerAscii(value[v]))) {
                ++p;
            ++v;
                } else if (p < pattern.size() && pattern[p] == '*') {
                    star_p = p++;
                    star_v = v;
                } else if (star_p != std::string_view::npos) {
                    p = star_p + 1;
                    v = ++star_v;
                } else {
                    return false;
                }
        }
        
        while (p < pattern.size() && pattern[p] == '*') {
            ++p;
        }
        
        return p == pattern.size();
    }
    
    std::optional<std::string> AssetEditorManager::DetectMimeType(
        std::string_view vfs_path
    ) const {
        if (!mMagic) {
            return std::nullopt;
        }
        
        auto file = mVFS.OpenFile(vfs_path);
        
        if (!file || !file->IsOpen()) {
            return std::nullopt;
        }
        
        const auto content = file->ReadToString();
        
        if (!content) {
            return std::nullopt;
        }
        
        const char* result = magic_buffer(
            mMagic,
            content->data(),
            content->size()
        );
        
        if (!result) {
            const char* err = magic_error(mMagic);
            
            CE_LOG(
                CECommon::LogLevel::Warn,
                "[AssetEditorManager] libmagic failed on '{}': {}",
                vfs_path,
                err ? err : "unknown error"
            );
            
            return std::nullopt;
        }
        
        std::string mime_type(result);
        
        const auto semicolon_pos = mime_type.find(';');
        
        if (semicolon_pos != std::string::npos) {
            mime_type.resize(semicolon_pos);
        }
        
        while (!mime_type.empty() &&
            std::isspace(static_cast<unsigned char>(mime_type.back()))) {
            mime_type.pop_back();
            }
            
            if (mime_type.empty()) {
                return std::nullopt;
            }
            
            return mime_type;
    }
    
    bool AssetEditorManager::CanOpen(std::string_view vfs_path) const {
        if (mOpenEditors.contains(std::string(vfs_path))) {
            return true;
        }
        
        const auto mime_type = DetectMimeType(vfs_path);
        
        return mime_type.has_value() &&
        FindFactory(*mime_type) != nullptr;
    }
    
    IAssetEditor* AssetEditorManager::Open(std::string_view vfs_path) {
        std::string key(vfs_path);
        
        auto it = mOpenEditors.find(key);
        
        if (it != mOpenEditors.end()) {
            return it->second.get();
        }
        
        const auto mime_type = DetectMimeType(vfs_path);
        
        if (!mime_type) {
            return nullptr;
        }
        
        const auto* factory = FindFactory(*mime_type);
        
        if (!factory) {
            return nullptr;
        }
        
        auto editor = (*factory)();
        
        if (!editor) {
            return nullptr;
        }
        
        editor->Open(vfs_path);
        
        auto* ptr = editor.get();
        
        mOpenEditors.emplace(
            std::move(key),
            std::move(editor)
        );
        
        return ptr;
    }
    
    void AssetEditorManager::Close(std::string_view vfs_path) {
        auto it = mOpenEditors.find(std::string(vfs_path));
        
        if (it == mOpenEditors.end()) {
            return;
        }
        
        it->second->Close();
        mOpenEditors.erase(it);
    }
    
    void AssetEditorManager::CloseAll() {
        for (auto& [path, editor] : mOpenEditors) {
            if (editor) {
                editor->Close();
            }
        }
        
        mOpenEditors.clear();
    }
    
    IAssetEditor* AssetEditorManager::Get(std::string_view vfs_path) const {
        auto it = mOpenEditors.find(std::string(vfs_path));
        
        if (it == mOpenEditors.end()) {
            return nullptr;
        }
        
        return it->second.get();
    }
    
    bool AssetEditorManager::IsOpen(std::string_view vfs_path) const {
        return mOpenEditors.contains(std::string(vfs_path));
    }
    
    bool AssetEditorManager::MatchScore::operator>(
        const MatchScore& other
    ) const {
        return std::tie(
            is_exact,
            literal_count,
            neg_star_count,
            neg_question_count,
            index
        ) > std::tie(
            other.is_exact,
            other.literal_count,
            other.neg_star_count,
            other.neg_question_count,
            other.index
        );
    }
    
    char AssetEditorManager::ToLowerAscii(char c) {
        return static_cast<char>(
            std::tolower(static_cast<unsigned char>(c))
        );
    }
    
    AssetEditorManager::MatchScore AssetEditorManager::ScorePattern(
        std::string_view pattern,
        size_t index
    ) {
        size_t literals = 0;
        size_t stars = 0;
        size_t questions = 0;
        
        for (const char ch : pattern) {
            if (ch == '*') {
                ++stars;
            } else if (ch == '?') {
                ++questions;
            } else {
                ++literals;
            }
        }
        
        return MatchScore{
            .is_exact = (stars == 0 && questions == 0),
            .literal_count = literals,
            .neg_star_count = -static_cast<std::ptrdiff_t>(stars),
            .neg_question_count = -static_cast<std::ptrdiff_t>(questions),
            .index = index,
        };
    }
    
    const AssetEditorManager::EditorFactory* AssetEditorManager::FindFactory(
        std::string_view mime_type
    ) const {
        const EditorFactory* best_factory = nullptr;
        MatchScore best_score{};
        
        for (size_t i = 0; i < mProviders.size(); ++i) {
            const auto& provider = mProviders[i];
            
            if (!MatchWildcard(provider.pattern, mime_type)) {
                continue;
            }
            
            const MatchScore score = ScorePattern(provider.pattern, i);
            
            if (!best_factory || score > best_score) {
                best_factory = &provider.factory;
                best_score = score;
            }
        }
        
        return best_factory;
    }
    
    
    bool AssetEditorManager::AddMagicDatabase(std::string_view vfs_path) {
        if (!mMagic || vfs_path.empty() || !mVFS.FileExists(vfs_path)) {
            return false;
        }
        
        auto file = mVFS.OpenFile(vfs_path);
        
        if (!file || !file->IsOpen()) {
            return false;
        }
        
        auto content = file->ReadToString();
        
        if (!content || content->empty()) {
            return false;
        }
        
        // Build a candidate collection without modifying the existing one.
        auto databases = mMagicDatabaseContents;
        databases.push_back(std::move(*content));
        
        // Create a separate magic instance so failed reloads
        // cannot invalidate the currently loaded databases.
        magic_t new_magic = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);
        
        if (!new_magic) {
            CE_LOG(
                CECommon::LogLevel::Error,
                "[AssetEditorManager] Failed to initialize libmagic"
            );
            
            return false;
        }
        
        std::vector<void*> buffers;
        std::vector<size_t> sizes;
        
        buffers.reserve(databases.size());
        sizes.reserve(databases.size());
        
        for (auto& database : databases) {
            buffers.push_back(database.data());
            sizes.push_back(database.size());
        }
        
        if (magic_load_buffers(
            new_magic,
            buffers.data(),
            sizes.data(),
            buffers.size()
        ) != 0) {
            const char* err = magic_error(new_magic);
            
            CE_LOG(
                CECommon::LogLevel::Error,
                "[AssetEditorManager] Failed to load magic database '{}': {}",
                vfs_path,
                err ? err : "unknown error"
            );
            
            magic_close(new_magic);
            return false;
        }
        
        // Replace the old instance only after successful loading.
        magic_close(mMagic);
        mMagic = new_magic;
        
        mMagicDatabaseContents = std::move(databases);
        
        return true;
    }
    
}
