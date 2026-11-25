#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <magic.h>

#include "ce_common/tracelog.hpp"
#include "ce_editor/ui/i_window.hpp"

namespace CE::Editor::Assets {
    class IAssetEditor : public UI::IWindow {
        public:
            using UI::IWindow::IWindow;
            virtual ~IAssetEditor() = default;

            virtual void Open(std::filesystem::path path) = 0;

            virtual void Close() = 0;
    };

    class AssetEditorManager {
        public:
            using EditorFactory = std::function<std::unique_ptr<IAssetEditor>()>;

            explicit AssetEditorManager(const std::filesystem::path& magic_db_path = {}) {
                mMagic = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);
                if (!mMagic) {
                    CE_LOG(CECommon::LogLevel::Error, "[AssetEditorManager] Failed to initialize libmagic");
                    return;
                }

                const char* db_path_cstr = nullptr;
                std::string db_path_str;
                std::error_code ec;
                if (!magic_db_path.empty() && std::filesystem::exists(magic_db_path, ec) && !ec) {
                    db_path_str = magic_db_path.string();
                    db_path_cstr = db_path_str.c_str();
                }

                if (magic_load(mMagic, db_path_cstr) != 0) {
                    const char* err = magic_error(mMagic);
                    if (db_path_cstr != nullptr) {
                        CE_LOG(
                            CECommon::LogLevel::Warn,
                            "[AssetEditorManager] Failed to load magic database '{}': {}; falling back to default",
                            db_path_str,
                            err ? err : "unknown error"
                        );
                        if (magic_load(mMagic, nullptr) == 0) {
                            return;
                        }
                        err = magic_error(mMagic);
                    }

                    CE_LOG(
                        CECommon::LogLevel::Error,
                        "[AssetEditorManager] Failed to load magic database: {}",
                        err ? err : "unknown error"
                    );
                    magic_close(mMagic);
                    mMagic = nullptr;
                }
            }

            ~AssetEditorManager() {
                CloseAll();
                if (mMagic) {
                    magic_close(mMagic);
                    mMagic = nullptr;
                }
            }

            AssetEditorManager(const AssetEditorManager&) = delete;
            AssetEditorManager& operator=(const AssetEditorManager&) = delete;

            AssetEditorManager(AssetEditorManager&& other) noexcept
                : mMagic(std::exchange(other.mMagic, nullptr))
                , mProviders(std::move(other.mProviders))
                , mOpenEditors(std::move(other.mOpenEditors)) {}

            AssetEditorManager& operator=(AssetEditorManager&& other) noexcept {
                if (this != &other) {
                    CloseAll();
                    if (mMagic) {
                        magic_close(mMagic);
                    }
                    mMagic = std::exchange(other.mMagic, nullptr);
                    mProviders = std::move(other.mProviders);
                    mOpenEditors = std::move(other.mOpenEditors);
                }
                return *this;
            }

            template<typename T>
            void Register(const std::string& mime_pattern) {
                Register(mime_pattern, []() {
                    return std::make_unique<T>();
                });
            }

            void Register(const std::string& mime_pattern, EditorFactory factory) {
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

            static bool MatchWildcard(std::string_view pattern, std::string_view value) {
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

            std::optional<std::string> DetectMimeType(const std::filesystem::path& path) const {
                if (!mMagic) {
                    return std::nullopt;
                }

                std::error_code ec;
                if (!std::filesystem::exists(path, ec) || ec) {
                    return std::nullopt;
                }

                const std::string path_str = path.string();
                const char* result = magic_file(mMagic, path_str.c_str());
                if (!result) {
                    const char* err = magic_error(mMagic);
                    CE_LOG(
                        CECommon::LogLevel::Warn,
                        "[AssetEditorManager] libmagic failed on '{}': {}",
                        path_str,
                        err ? err : "unknown error"
                    );
                    return std::nullopt;
                }

                std::string mime_type(result);
                const auto semicolon_pos = mime_type.find(';');
                if (semicolon_pos != std::string::npos) {
                    mime_type.resize(semicolon_pos);
                }

                while (!mime_type.empty() && std::isspace(static_cast<unsigned char>(mime_type.back()))) {
                    mime_type.pop_back();
                }

                if (mime_type.empty()) {
                    return std::nullopt;
                }

                return mime_type;
            }

            bool CanOpen(const std::filesystem::path& path) const {
                if (mOpenEditors.contains(path)) {
                    return true;
                }

                const auto mime_type = DetectMimeType(path);
                return mime_type.has_value() && FindFactory(*mime_type) != nullptr;
            }

            IAssetEditor* Open(const std::filesystem::path& path) {
                auto it = mOpenEditors.find(path);
                if (it != mOpenEditors.end()) {
                    return it->second.get();
                }

                const auto mime_type = DetectMimeType(path);
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

                editor->Open(path);

                auto* ptr = editor.get();
                mOpenEditors.emplace(path, std::move(editor));

                return ptr;
            }

            void Close(const std::filesystem::path& path) {
                auto it = mOpenEditors.find(path);
                if (it == mOpenEditors.end()) {
                    return;
                }

                it->second->Close();
                mOpenEditors.erase(it);
            }

            void CloseAll() {
                for (auto& [path, editor] : mOpenEditors) {
                    if (editor) {
                        editor->Close();
                    }
                }
                mOpenEditors.clear();
            }

            IAssetEditor* Get(const std::filesystem::path& path) const {
                auto it = mOpenEditors.find(path);
                if (it == mOpenEditors.end()) {
                    return nullptr;
                }

                return it->second.get();
            }

            bool IsOpen(const std::filesystem::path& path) const {
                return mOpenEditors.contains(path);
            }

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

                bool operator>(const MatchScore& other) const {
                    return std::tie(is_exact, literal_count, neg_star_count, neg_question_count, index) >
                           std::tie(other.is_exact, other.literal_count, other.neg_star_count, other.neg_question_count, other.index);
                }
            };

            static char ToLowerAscii(char c) {
                return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }

            static MatchScore ScorePattern(std::string_view pattern, size_t index) {
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

            const EditorFactory* FindFactory(std::string_view mime_type) const {
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

            magic_t mMagic = nullptr;
            std::vector<ProviderEntry> mProviders;
            std::unordered_map<std::filesystem::path, std::unique_ptr<IAssetEditor>> mOpenEditors;
    };
}
