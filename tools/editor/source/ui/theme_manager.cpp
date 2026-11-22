#include <format>
#include <system_error>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

#include "ce_runtime/common/tracelog.hpp"
#include "ce_editor/ui/theme_manager.hpp"
#include "ce_runtime/common/fs/file_provider.hpp"
#include "ce_common/tracelog.hpp"

namespace CE::Editor::UI {
    ThemeManager::ThemeManager(
        Renderer::IRenderer& renderer, 
        Common::FS::VFS::VFS& vfs, 
        const std::string& theme
    ) : mRenderer(renderer), mVFS(vfs) {

    }

    Status ThemeManager::ReloadThemeCache() {
        // Find all inbuilt themes
        {
            if (!mVFS.DirExists("/themes")) {
                return Error {
                    .message = "Inbuilt theme directory was not found! Is the installation corrupt?",
                    .context = "ThemeManager::ReloadThemeCache()",
                    .code = std::make_error_code(std::errc::no_such_file_or_directory),
                }; 
            }

            auto inbuilt_theme_dir_list = mVFS.ListDirectory("/themes");
            if (inbuilt_theme_dir_list.empty()) {
                return Error {
                    .message = "Inbuilt theme directory was empty, is the installation corrupt?",
                    .context = "ThemeManager::ReloadThemeCache()",  
                };
            }

            for (const auto& dir_content : inbuilt_theme_dir_list) {
                if (dir_content.type == Common::FS::VFS::DirectoryContent::Type::File) continue;

                if (!mVFS.FileExists(std::string("/themes/" + dir_content.name + "/theme.yaml"))) {
                    CE_LOG(
                        LogLevel::Warn,
                        "[ThemeManager] Inbuilt theme directory '{}' exists but has no theme.yaml at '/themes/{}/theme.yaml'. This is probably an issue.",
                        dir_content.name,
                        dir_content.name
                    );
                    continue;
                }

                auto theme_file = mVFS.OpenFile(std::format("/themes/{}/theme.yaml", dir_content.name));
                if (!theme_file) {
                    CE_LOG(LogLevel::Warn, "[ThemeManager] Failed to open theme file: /themes/{}/theme.yaml", dir_content.name);
                    continue;
                }

                auto theme_file_data = theme_file->ReadToString();
                if (!theme_file_data) {
                    CE_LOG(LogLevel::Warn, "[ThemeManager] Failed to read theme file: '/themes/{}/theme.yaml' to a string", dir_content.name);
                    continue;
                }

                try {
                    YAML::Node in = YAML::Load(*theme_file_data);

                    
                } catch (const std::exception& e) {
                    CE_LOG(LogLevel::Warn, "[ThemeManager] YAML Error in file '/themes/{}/theme.yaml', {} ", dir_content.name, e.what());
                    continue;
                }
            }
        }


        return {};
    }
}