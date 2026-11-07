
#include "platforms.hpp"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace Platforms {
    namespace {
        constexpr const char* REGISTRY_KEY = "Environment";
        constexpr const char* REGISTRY_VALUE = "Path";

        std::string NormalizePath(const std::string& path) {
            std::string result = path;

            while (result.size() > 3 &&
                   (result.back() == '\\' || result.back() == '/')) {
                result.pop_back();
            }

            std::replace(result.begin(), result.end(), '/', '\\');

            return result;
        }

        bool EqualPath(const std::string& a, const std::string& b) {
            return _stricmp(
                NormalizePath(a).c_str(),
                NormalizePath(b).c_str()
            ) == 0;
        }

        std::vector<std::string> SplitPath(const std::string& path) {
            std::vector<std::string> entries;
            size_t start = 0;

            while (start <= path.size()) {
                size_t end = path.find(';', start);

                if (end == std::string::npos) {
                    end = path.size();
                }

                std::string entry = path.substr(start, end - start);

                if (!entry.empty()) {
                    entries.push_back(entry);
                }

                start = end + 1;
            }

            return entries;
        }

        bool ReadUserPath(std::string& path) {
            HKEY key = nullptr;

            if (RegOpenKeyExA(
                HKEY_CURRENT_USER,
                REGISTRY_KEY,
                0,
                KEY_QUERY_VALUE,
                &key
            ) != ERROR_SUCCESS) {
                return false;
            }

            DWORD type = 0;
            DWORD size = 0;

            LONG result = RegQueryValueExA(
                key,
                REGISTRY_VALUE,
                nullptr,
                &type,
                nullptr,
                &size
            );

            if (result == ERROR_FILE_NOT_FOUND) {
                path.clear();
                RegCloseKey(key);
                return true;
            }

            if (result != ERROR_SUCCESS ||
                (type != REG_SZ && type != REG_EXPAND_SZ)) {
                RegCloseKey(key);
                return false;
            }

            std::vector<char> buffer(size + 1, '\0');

            result = RegQueryValueExA(
                key,
                REGISTRY_VALUE,
                nullptr,
                &type,
                reinterpret_cast<BYTE*>(buffer.data()),
                &size
            );

            RegCloseKey(key);

            if (result != ERROR_SUCCESS) {
                return false;
            }

            path = buffer.data();
            return true;
        }

        bool WriteUserPath(const std::string& path) {
            HKEY key = nullptr;

            if (RegCreateKeyExA(
                HKEY_CURRENT_USER,
                REGISTRY_KEY,
                0,
                nullptr,
                0,
                KEY_SET_VALUE,
                nullptr,
                &key,
                nullptr
            ) != ERROR_SUCCESS) {
                return false;
            }

            LONG result = RegSetValueExA(
                key,
                REGISTRY_VALUE,
                0,
                REG_EXPAND_SZ,
                reinterpret_cast<const BYTE*>(path.c_str()),
                static_cast<DWORD>(path.size() + 1)
            );

            RegCloseKey(key);

            if (result != ERROR_SUCCESS) {
                return false;
            }

            SendMessageTimeoutA(
                HWND_BROADCAST,
                WM_SETTINGCHANGE,
                0,
                reinterpret_cast<LPARAM>("Environment"),
                SMTO_ABORTIFHUNG,
                5000,
                nullptr
            );

            return true;
        }

        void UpdateCurrentProcess(const std::string& path) {
            SetEnvironmentVariableA("PATH", path.c_str());
        }
    }

    bool IsDirInPath(std::string path) {
        const char* current = std::getenv("PATH");

        if (!current || path.empty()) {
            return false;
        }

        for (const auto& entry : SplitPath(current)) {
            if (EqualPath(entry, path)) {
                return true;
            }
        }

        return false;
    }

    bool AddDirToPath(std::string path) {
        if (path.empty()) {
            return false;
        }

        std::string current;

        if (!ReadUserPath(current)) {
            return false;
        }

        for (const auto& entry : SplitPath(current)) {
            if (EqualPath(entry, path)) {
                return true;
            }
        }

        std::string updated = path;

        if (!current.empty()) {
            updated += ";" + current;
        }

        if (!WriteUserPath(updated)) {
            return false;
        }

        UpdateCurrentProcess(updated);

        return true;
    }

    bool RemoveDirFromPath(std::string path) {
        if (path.empty()) {
            return false;
        }

        std::string current;

        if (!ReadUserPath(current)) {
            return false;
        }

        auto entries = SplitPath(current);

        std::vector<std::string> updatedEntries;
        bool removed = false;

        for (const auto& entry : entries) {
            if (EqualPath(entry, path)) {
                removed = true;
                continue;
            }

            updatedEntries.push_back(entry);
        }

        if (!removed) {
            return true;
        }

        std::string updated;

        for (const auto& entry : updatedEntries) {
            if (!updated.empty()) {
                updated += ";";
            }

            updated += entry;
        }

        if (!WriteUserPath(updated)) {
            return false;
        }

        UpdateCurrentProcess(updated);

        return true;
    }

    std::string GetExecutablePath() {
        std::vector<wchar_t> buffer(512);

        while (true) {
            DWORD length = GetModuleFileNameW(
                nullptr,
                buffer.data(),
                static_cast<DWORD>(buffer.size())
            );

            if (length == 0) {
                return {};
            }

            if (length < buffer.size()) {
                int required = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    buffer.data(),
                    static_cast<int>(length),
                    nullptr,
                    0,
                    nullptr,
                    nullptr
                );

                if (required <= 0) {
                    return {};
                }

                std::string result(required, '\0');

                if (WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    buffer.data(),
                    static_cast<int>(length),
                    result.data(),
                    required,
                    nullptr,
                    nullptr
                ) <= 0) {
                    return {};
                }

                return result;
            }

            buffer.resize(buffer.size() * 2);
        }
    }
}