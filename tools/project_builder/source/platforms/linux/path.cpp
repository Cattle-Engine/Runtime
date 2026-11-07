#include "platforms.hpp"


#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Platforms {
    namespace {
        namespace fs = std::filesystem;

        std::string GetProfilePath() {
            const char* home = std::getenv("HOME");

            if (!home || !*home) {
                return {};
            }

            return (fs::path(home) / ".profile").string();
        }

        bool ValidDirectory(const std::string& directory) {
            return !directory.empty() &&
                   directory.find('\0') == std::string::npos &&
                   directory.find('\n') == std::string::npos &&
                   directory.find('\r') == std::string::npos;
        }

        std::string NormalizePath(const std::string& path) {
            return fs::path(path).lexically_normal().string();
        }

        std::vector<std::string> SplitPath(const std::string& path) {
            std::vector<std::string> entries;
            size_t start = 0;

            while (start <= path.size()) {
                size_t end = path.find(':', start);

                if (end == std::string::npos) {
                    end = path.size();
                }

                if (end > start) {
                    entries.push_back(path.substr(start, end - start));
                }

                start = end + 1;
            }

            return entries;
        }

        std::string ShellQuote(const std::string& value) {
            std::string result = "'";

            for (char c : value) {
                if (c == '\'') {
                    result += "'\\''";
                } else {
                    result += c;
                }
            }

            result += "'";
            return result;
        }

        std::string BeginMarker(const std::string& directory) {
            return "# >>> Platforms PATH BEGIN: " + directory;
        }

        std::string EndMarker(const std::string& directory) {
            return "# <<< Platforms PATH END: " + directory;
        }

        std::string CreateBlock(const std::string& directory) {
            std::string quoted = ShellQuote(directory);

            std::string block;

            block += BeginMarker(directory) + "\n";
            block += "case \":${PATH-}:\" in\n";
            block += "    *:" + quoted + ":*) ;;\n";
            block += "    *) PATH=" + quoted + "${PATH:+\":$PATH\"} ;;\n";
            block += "esac\n";
            block += "export PATH\n";
            block += EndMarker(directory) + "\n";

            return block;
        }

        bool ReadFile(const std::string& filename, std::string& contents) {
            std::ifstream file(filename, std::ios::binary);

            if (!file.is_open()) {
                if (errno == ENOENT) {
                    contents.clear();
                    return true;
                }

                return false;
            }

            contents.assign(
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            );

            return file.good() || file.eof();
        }

        bool WriteFileAtomically(
            const std::string& filename,
            const std::string& contents
        ) {
            struct stat st {};
            mode_t permissions = 0600;

            if (stat(filename.c_str(), &st) == 0) {
                if (!S_ISREG(st.st_mode)) {
                    return false;
                }

                permissions = st.st_mode & 0777;
            } else if (errno != ENOENT) {
                return false;
            }

            std::string temporary = filename + ".platforms.XXXXXX";
            std::vector<char> tempName(
                temporary.begin(),
                temporary.end()
            );

            tempName.push_back('\0');

            int fd = mkstemp(tempName.data());

            if (fd < 0) {
                return false;
            }

            bool success = fchmod(fd, permissions) == 0;

            size_t written = 0;

            while (success && written < contents.size()) {
                ssize_t result = write(
                    fd,
                    contents.data() + written,
                    contents.size() - written
                );

                if (result < 0) {
                    if (errno == EINTR) {
                        continue;
                    }

                    success = false;
                    break;
                }

                if (result == 0) {
                    success = false;
                    break;
                }

                written += static_cast<size_t>(result);
            }

            if (success && fsync(fd) != 0) {
                success = false;
            }

            if (close(fd) != 0) {
                success = false;
            }

            if (success) {
                if (rename(tempName.data(), filename.c_str()) != 0) {
                    success = false;
                }
            }

            if (!success) {
                unlink(tempName.data());
            }

            return success;
        }

        bool AddProfileEntry(const std::string& directory) {
            std::string filename = GetProfilePath();

            if (filename.empty()) {
                return false;
            }

            std::string contents;

            if (!ReadFile(filename, contents)) {
                return false;
            }

            std::string begin = BeginMarker(directory);

            if (contents.find(begin) != std::string::npos) {
                return true;
            }

            if (!contents.empty() && contents.back() != '\n') {
                contents += '\n';
            }

            contents += CreateBlock(directory);

            return WriteFileAtomically(filename, contents);
        }

        bool RemoveProfileEntry(const std::string& directory) {
            std::string filename = GetProfilePath();

            if (filename.empty()) {
                return false;
            }

            std::string contents;

            if (!ReadFile(filename, contents)) {
                return false;
            }

            std::string begin = BeginMarker(directory);
            std::string end = EndMarker(directory);

            size_t start = contents.find(begin);

            if (start == std::string::npos) {
                return true;
            }

            size_t finish = contents.find(end, start);

            if (finish == std::string::npos) {
                return false;
            }

            finish += end.size();

            if (finish < contents.size() && contents[finish] == '\n') {
                ++finish;
            }

            contents.erase(start, finish - start);

            return WriteFileAtomically(filename, contents);
        }

        void UpdateCurrentProcess(const std::string& directory, bool add) {
            const char* current = std::getenv("PATH");
            std::vector<std::string> entries =
                SplitPath(current ? current : "");

            std::vector<std::string> updated;

            for (const auto& entry : entries) {
                if (!add && NormalizePath(entry) == NormalizePath(directory)) {
                    continue;
                }

                updated.push_back(entry);
            }

            if (add) {
                bool exists = false;

                for (const auto& entry : updated) {
                    if (NormalizePath(entry) == NormalizePath(directory)) {
                        exists = true;
                        break;
                    }
                }

                if (!exists) {
                    updated.insert(updated.begin(), directory);
                }
            }

            std::string result;

            for (const auto& entry : updated) {
                if (!result.empty()) {
                    result += ":";
                }

                result += entry;
            }

            setenv("PATH", result.c_str(), 1);
        }
    }

    bool IsDirInPath(std::string path) {
        if (!ValidDirectory(path)) {
            return false;
        }

        const char* current = std::getenv("PATH");

        if (!current) {
            return false;
        }

        for (const auto& entry : SplitPath(current)) {
            if (NormalizePath(entry) == NormalizePath(path)) {
                return true;
            }
        }

        return false;
    }

    bool AddDirToPath(std::string path) {
        if (!ValidDirectory(path)) {
            return false;
        }

        if (!AddProfileEntry(path)) {
            return false;
        }

        UpdateCurrentProcess(path, true);

        return true;
    }

    bool RemoveDirFromPath(std::string path) {
        if (!ValidDirectory(path)) {
            return false;
        }

        if (!RemoveProfileEntry(path)) {
            return false;
        }

        UpdateCurrentProcess(path, false);

        return true;
    }

    std::string GetExecutablePath() {
        std::vector<char> buffer(512);

        while (true) {
            ssize_t length = readlink(
                "/proc/self/exe",
                buffer.data(),
                buffer.size()
            );

            if (length < 0) {
                return {};
            }

            if (static_cast<size_t>(length) < buffer.size()) {
                return std::string(buffer.data(), length);
            }

            buffer.resize(buffer.size() * 2);
        }
    }
}