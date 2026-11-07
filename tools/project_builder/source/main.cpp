#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#else
  #include <sys/wait.h>
  #include <unistd.h>
#endif

#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>

#include "platforms.hpp"
#include "yaml-cpp/yaml.h"

#include "gameinfo.hpp"
#include <yaml-cpp/exceptions.h>

constexpr std::string_view kVersionText = "0.1.0";
constexpr uint32_t kSupportedManifestVersion = 1;
constexpr uint32_t kSupportedProjectVersion = 1;

constexpr const char* kReleaseManifestUrl = "https://raw.githubusercontent.com/Cattle-Engine/Runtime/refs/heads/main/ce_distribution_manifest.yaml";
constexpr const char* kLocalManifestName = "toolchain_manifest.yaml";
constexpr const char* kToolchainDirName = "toolchains";
constexpr const char* kProjectManifestName = "ce_project.yaml";
constexpr const char* kLocalToolsManifestName = "tools_manifest.yaml";
constexpr const char* kToolsDirName = "tools";

// Layout of an installed toolchain (relative to its root). Change these if
// your toolchain archives are laid out differently.
constexpr const char* kToolchainBinDir = "bin";                       // ce_scriptc, ce_tcf
constexpr const char* kToolchainRuntimeDir = "bin";               // ce_loader[.exe], ce_runtime, ce_common
constexpr const char* kToolchainShaderDir = "shaders/Compiled/SPIRV"; // optional
    
namespace FS = std::filesystem;

// Must match the "platform" strings used in ce_release_manifest.yaml.
constexpr std::string_view kKnownPlatforms[] = {
    "windows-x64", "windows-arm64",
    "macos-x64",   "macos-arm64",
    "linux-x64",   "linux-arm64",
};

enum class CEGrillerReturns {
    ArgumentParseFail,
    CreateNewProjectFail,
    BuildProjectFail,
    ToolchainInstallFail,
    ToolchainUpdateFail,
    ToolchainListFail,
    ToolchainPathFail,  
    Okay,
};


enum class CEGrillerCommand {
    None,
    Build, 
    New,
    ToolchainInstall, 
    ToolchainUpdate, 
    ToolchainList,
    ToolchainPathAdd,      
    ToolchainPathRemove,  
    ToolchainPathStatus,  
};
template <typename T>
constexpr std::uint32_t ToUint32(T value) {
    return static_cast<std::uint32_t>(value);
}

struct ShaderConfig {
    FS::path source_dir = "shaders";      // relative to the project root
    std::string compiler = "glslc";       // tool name in the release manifest
    std::string compiler_version = "latest";
    std::string target_env;               // e.g. vulkan1.2 (empty = compiler default)
    std::vector<FS::path> include_dirs;   // relative to the project root
};

struct CEBuildInfo {
    uint32_t manifest_version = 0;

    std::vector<FS::path> asset_paths;
    std::vector<std::string> exclude_files;

    FS::path script_dir;
    ShaderConfig shaders;
    FS::path build_output;
    std::string game_data_file_name;
    std::string toolchain_needed;
    CE::GameInfo gameinfo;
};

struct Options {
    CEGrillerCommand command = CEGrillerCommand::None;

    bool show_help = false;
    bool show_version = false;

    bool debug = false;          // -g for scriptc, verbose commands, keep staging
    bool keep_staging = false;

    std::string version;
    std::string platform;        // empty = host platform
    FS::path project_root;
};

void ShowHelp() {
    std::cout << "Usage: ce_griller <COMMAND> [OPTIONS]\n"
              << "Commands:\n"
              << "  build <PROJECT_PATH>        Builds a CE project\n"
              << "  new <PROJECT_PATH>          Creates a new CE project\n"
              << "  toolchain install <VERSION> Installs a toolchain version ('latest' is accepted)\n"
              << "  toolchain update            Installs the newest toolchain if you don't have it\n"
              << "  toolchain list              Lists installed toolchains\n"
              << "  toolchain path add <VERSION>    Puts a toolchain's bin directory on your user PATH\n"
              << "                                  ('latest' is accepted; replaces any other ce_griller toolchain)\n"
              << "  toolchain path remove <VERSION> Removes it from your user PATH ('all' removes every one)\n"
              << "  toolchain path status           Shows which toolchains are on your PATH\n"
              << "Options:\n"
              << "  -p, --platform <PLATFORM>   Target platform (build) / platform to fetch (toolchain\n"
              << "                              install/update). Defaults to this machine.\n"
              << "                              One of: windows-x64 windows-arm64 macos-x64\n"
              << "                                      macos-arm64 linux-x64 linux-arm64\n"
              << "  -d, --debug                 Debug build: script debug info (-g), echo every\n"
              << "                              command, keep the staging directory\n"
              << "      --keep-staging          Keep the staging directory after building\n"
              << "  -h, --help                  Shows this help message\n"
              << "  -v, --version               Shows the version\n";
}

void PrintVersion() {
    std::cout << "ce_griller " << kVersionText << "\n";
}

bool ParseArguments(int argc, char* argv[], Options& options) {
    std::vector<std::string_view> positional;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);

        if (arg == "-h" || arg == "--help") {
            options.show_help = true;
            continue;
        }

        if (arg == "-v" || arg == "--version") {
            options.show_version = true;
            continue;
        }

        if (arg == "-d" || arg == "--debug") {
            options.debug = true;
            continue;
        }

        if (arg == "--keep-staging") {
            options.keep_staging = true;
            continue;
        }

        if (arg == "-p" || arg == "--platform") {
            if (i + 1 >= argc) {
                std::cerr << "ce_griller: " << arg << " expects a platform\n";
                return false;
            }
            options.platform = argv[++i];
            continue;
        }

        if (arg.substr(0, 11) == "--platform=") {
            options.platform = std::string(arg.substr(11));
            continue;
        }

        if (arg.size() > 1 && arg.front() == '-') {
            std::cerr << "ce_griller: unknown option '" << arg << "'\n";
            return false;
        }

        positional.emplace_back(arg);
    }

    if (options.show_help || options.show_version) {
        return true;
    }

    if (!options.platform.empty()) {
        const bool known = std::any_of(std::begin(kKnownPlatforms), std::end(kKnownPlatforms),
                                       [&](std::string_view p) { return p == options.platform; });
        if (!known) {
            std::cerr << "ce_griller: unknown platform '" << options.platform << "'\n";
            return false;
        }
    }

    if (positional.empty()) {
        std::cerr << "ce_griller: expected a command\n";
        return false;
    }

    const std::string_view command = positional[0];

    if (command == "build") {
        options.command = CEGrillerCommand::Build;

        if (positional.size() != 2) {
            std::cerr << "ce_griller: expected exactly one project directory\n";
            return false;
        }

        options.project_root = positional[1];
        return true;
    }

    if (command == "new") {
        options.command = CEGrillerCommand::New;

        if (positional.size() != 2) {
            std::cerr << "ce_griller: expected exactly one project directory\n";
            return false;
        }

        options.project_root = positional[1];
        return true;
    }

    if (command == "toolchain") {
        if (positional.size() < 2) {
            std::cerr << "ce_griller: expected a toolchain command\n";
            return false;
        }

        const std::string_view subcommand = positional[1];

        if (subcommand == "install") {
            options.command = CEGrillerCommand::ToolchainInstall;

            if (positional.size() != 3) {
                std::cerr << "ce_griller: expected a toolchain version\n";
                return false;
            }

            options.version = positional[2];
            return true;
        }

        if (subcommand == "update") {
            options.command = CEGrillerCommand::ToolchainUpdate;

            if (positional.size() != 2) {
                std::cerr << "ce_griller: unexpected arguments for toolchain update\n";
                return false;
            }

            return true;
        }

        if (subcommand == "list") {
            options.command = CEGrillerCommand::ToolchainList;

            if (positional.size() != 2) {
                std::cerr << "ce_griller: unexpected arguments for toolchain list\n";
                return false;
            }

            return true;
        }

        if (subcommand == "path") {
            if (positional.size() < 3) {
                std::cerr << "ce_griller: expected 'add', 'remove' or 'status'\n";
                return false;
            }
            const std::string_view action = positional[2];

            if (action == "status") {
                options.command = CEGrillerCommand::ToolchainPathStatus;
                if (positional.size() != 3) {
                    std::cerr << "ce_griller: unexpected arguments for toolchain path status\n";
                    return false;
                }
                return true;
            }
            if (action == "add" || action == "remove") {
                options.command = (action == "add") ? CEGrillerCommand::ToolchainPathAdd
                                                    : CEGrillerCommand::ToolchainPathRemove;
                if (positional.size() != 4) {
                    std::cerr << "ce_griller: expected a toolchain version\n";
                    return false;
                }
                options.version = positional[3];
                return true;
            }
            std::cerr << "ce_griller: unknown toolchain path command '" << action << "'\n";
            return false;
        }

        std::cerr << "ce_griller: unknown toolchain command '"
                  << subcommand << "'\n";
        return false;
    }

    std::cerr << "ce_griller: unknown command '" << command << "'\n";
    return false;
}

// Small helpers
namespace {

FS::path ExeDir() {
    FS::path p(Platforms::GetExecutablePath());
    std::error_code ec;
    if (FS::is_directory(p, ec)) return p;
    return p.parent_path();
}

std::string CurrentPlatform() {
#if defined(_WIN32)
  #if defined(_M_ARM64) || defined(__aarch64__)
    return "windows-arm64";
  #else
    return "windows-x64";
  #endif
#elif defined(__APPLE__)
  #if defined(__aarch64__) || defined(__arm64__)
    return "macos-arm64";
  #else
    return "macos-x64";
  #endif
#elif defined(__linux__)
  #if defined(__aarch64__)
    return "linux-arm64";
  #else
    return "linux-x64";
  #endif
#else
    return "unknown";
#endif
}

// Platform requested on the command line, or the host if none.
std::string TargetPlatform(const Options& options) {
    return options.platform.empty() ? CurrentPlatform() : options.platform;
}

// Versions end up in directory names, so keep them boring.
bool IsSafeVersion(const std::string& v) {
    if (v.empty() || v.front() == '.') return false;
    return std::all_of(v.begin(), v.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == '+';
    });
}

bool IsSafeEntryPath(const FS::path& p) {
    if (p.empty() || p.is_absolute() || p.has_root_name() || p.has_root_directory()) return false;
    for (const auto& c : p) {
        if (c == "..") return false;
    }
    return true;
}

std::vector<uint32_t> ParseVersion(const std::string& v) {
    std::vector<uint32_t> parts;
    std::string core = v.substr(0, v.find_first_of("-+"));
    std::stringstream ss(core);
    std::string item;
    while (std::getline(ss, item, '.')) {
        try {
            parts.push_back(static_cast<uint32_t>(std::stoul(item)));
        } catch (...) {
            parts.push_back(0);
        }
    }
    return parts;
}

bool VersionLess(const std::string& a, const std::string& b) {
    return ParseVersion(a) < ParseVersion(b);  // lexicographic over numeric parts
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

template <typename T>
T Required(const YAML::Node& node, const char* key) {
    const YAML::Node v = node[key];
    if (!v) {
        throw std::runtime_error(std::string("missing required key '") + key + "'");
    }
    return v.as<T>();
}

template <typename T>
T Optional(const YAML::Node& node, const char* key, T fallback) {
    const YAML::Node v = node[key];
    return v ? v.as<T>() : fallback;
}

// Running external tools

std::string QuoteArg(const std::string& a) {
    if (!a.empty() && a.find_first_of(" \t\"") == std::string::npos) return a;
    std::string out = "\"";
    for (char c : a) {
        if (c == '"') out += '\\';
        out += c;
    }
    out += '"';
    return out;
}

// Returns true if the process ran and exited with code 0.
bool RunProcess(const std::vector<std::string>& args, bool echo) {
    if (args.empty()) return false;

    if (echo) {
        std::cout << "  $";
        for (const auto& a : args) std::cout << ' ' << QuoteArg(a);
        std::cout << "\n";
    }

#ifdef _WIN32
    std::string cmd;
    for (const auto& a : args) {
        if (!cmd.empty()) cmd += ' ';
        cmd += QuoteArg(a);
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        std::cerr << "ce_griller: failed to launch '" << args[0] << "' (error "
                  << GetLastError() << ")\n";
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return code == 0;
#else
    std::vector<char*> argv;
    for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    const pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "ce_griller: fork failed\n";
        return false;
    }
    if (pid == 0) {
        execvp(argv[0], argv.data());
        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            std::cerr << "ce_griller: waitpid failed\n";
            return false;
        }
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
        std::cerr << "ce_griller: could not execute '" << args[0] << "'\n";
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

struct CurlGlobal {
    CurlGlobal() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobal() { curl_global_cleanup(); }
};

size_t WriteToStream(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::ostream*>(userdata);
    out->write(ptr, static_cast<std::streamsize>(size * nmemb));
    return out->good() ? size * nmemb : 0;
}

int ProgressCallback(void*, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t) {
    if (dltotal > 0) {
        const int pct = static_cast<int>((dlnow * 100) / dltotal);
        std::cerr << "\r  downloading... " << pct << "%" << std::flush;
    }
    return 0;
}

bool HttpGet(const std::string& url, std::ostream& out, bool show_progress) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "ce_griller: failed to initialise libcurl\n";
        return false;
    }

    char error_buffer[CURL_ERROR_SIZE] = {};
    const std::string user_agent = "ce_griller/" + std::string(kVersionText);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToStream);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
#if LIBCURL_VERSION_NUM >= 0x075500
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
#endif
    if (show_progress) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
    }

    const CURLcode rc = curl_easy_perform(curl);
    if (show_progress) std::cerr << "\n";
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        std::cerr << "ce_griller: download failed: "
                  << (error_buffer[0] ? error_buffer : curl_easy_strerror(rc)) << "\n";
        return false;
    }
    return true;
}

bool HttpGetString(const std::string& url, std::string& result) {
    std::ostringstream ss;
    if (!HttpGet(url, ss, false)) return false;
    result = ss.str();
    return true;
}

bool HttpGetFile(const std::string& url, const FS::path& dest) {
    std::ofstream file(dest, std::ios::binary | std::ios::trunc);
    if (!file) {
        std::cerr << "ce_griller: cannot write '" << dest.string() << "'\n";
        return false;
    }
    const bool ok = HttpGet(url, file, true);
    file.close();
    return ok && !file.fail();
}

// libarchive
bool CopyData(archive* in, archive* out) {
    const void* buff;
    size_t size;
    la_int64_t offset;

    for (;;) {
        const int r = archive_read_data_block(in, &buff, &size, &offset);
        if (r == ARCHIVE_EOF) return true;
        if (r < ARCHIVE_OK) {
            std::cerr << "ce_griller: read error: " << archive_error_string(in) << "\n";
            return false;
        }
        if (archive_write_data_block(out, buff, size, offset) < ARCHIVE_OK) {
            std::cerr << "ce_griller: write error: " << archive_error_string(out) << "\n";
            return false;
        }
    }
}

bool ExtractArchive(const FS::path& archive_path, const FS::path& dest) {
    archive* reader = archive_read_new();
    archive_read_support_filter_all(reader);
    archive_read_support_format_all(reader);

    archive* writer = archive_write_disk_new();
    const int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
                      ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                      ARCHIVE_EXTRACT_SECURE_SYMLINKS |
    archive_write_disk_set_options(writer, flags);
    archive_write_disk_set_standard_lookup(writer);

#ifdef _WIN32
    int r = archive_read_open_filename_w(reader, archive_path.c_str(), 1 << 16);
#else
    int r = archive_read_open_filename(reader, archive_path.c_str(), 1 << 16);
#endif
    if (r != ARCHIVE_OK) {
        std::cerr << "ce_griller: cannot open archive: " << archive_error_string(reader) << "\n";
        archive_read_free(reader);
        archive_write_free(writer);
        return false;
    }

    bool ok = true;
    archive_entry* entry = nullptr;

    for (;;) {
        r = archive_read_next_header(reader, &entry);
        if (r == ARCHIVE_EOF) break;
        if (r < ARCHIVE_WARN) {
            std::cerr << "ce_griller: archive error: " << archive_error_string(reader) << "\n";
            ok = false;
            break;
        }

#ifdef _WIN32
        const wchar_t* name = archive_entry_pathname_w(entry);
#else
        const char* name = archive_entry_pathname(entry);
#endif
        if (!name) continue;

        if (!IsSafeEntryPath(FS::path(name))) {
            std::cerr << "ce_griller: refusing unsafe archive entry\n";
            ok = false;
            break;
        }

        const FS::path full = dest / name;
#ifdef _WIN32
        archive_entry_copy_pathname_w(entry, full.c_str());
#else
        archive_entry_set_pathname(entry, full.c_str());
#endif

        if (const char* link = archive_entry_hardlink(entry)) {
            if (!IsSafeEntryPath(FS::path(link))) { ok = false; break; }
            archive_entry_set_hardlink(entry, (dest / link).string().c_str());
        }
        r = archive_write_header(writer, entry);
        if (r < ARCHIVE_WARN) {
            std::cerr << "ce_griller: " << archive_error_string(writer) << "\n";
            ok = false;
            break;
        }
        if (archive_entry_size(entry) > 0) {
            if (!CopyData(reader, writer)) {
                ok = false;
                break;
            }
        }
        if (archive_write_finish_entry(writer) < ARCHIVE_WARN) {
            std::cerr << "ce_griller: " << archive_error_string(writer) << "\n";
            ok = false;
            break;
        }
    }

    archive_read_close(reader);
    archive_read_free(reader);
    archive_write_close(writer);
    archive_write_free(writer);
    return ok;
}

// Installed toolchains

struct InstalledToolchain {
    std::string version;
    std::string path;      // relative to the executable directory
    std::string platform;
};

bool LoadInstalled(std::vector<InstalledToolchain>& out) {
    out.clear();
    const FS::path manifest_path = ExeDir() / kLocalManifestName;

    if (!FS::exists(manifest_path)) return true;  // nothing installed yet

    try {
        const YAML::Node manifest = YAML::LoadFile(manifest_path.string());

        if (!manifest["manifest_version"] ||
            manifest["manifest_version"].as<uint32_t>() != kSupportedManifestVersion) {
            std::cerr << "ce_griller: unsupported toolchain manifest version\n";
            return false;
        }

        const YAML::Node toolchains = manifest["toolchains"];
        if (!toolchains || !toolchains.IsSequence()) {
            std::cerr << "ce_griller: invalid toolchain manifest\n";
            return false;
        }

        for (const YAML::Node& t : toolchains) {
            if (!t.IsMap() || !t["version"] || !t["path"] || !t["platform"]) {
                std::cerr << "ce_griller: invalid toolchain entry\n";
                return false;
            }
            out.push_back({t["version"].as<std::string>(),
                           t["path"].as<std::string>(),
                           t["platform"].as<std::string>()});
        }
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "ce_griller: failed to read toolchain manifest: " << e.what() << "\n";
        return false;
    }
}

bool SaveInstalled(const std::vector<InstalledToolchain>& list) {
    YAML::Emitter e;
    e << YAML::BeginMap;
    e << YAML::Key << "manifest_version" << YAML::Value << kSupportedManifestVersion;
    e << YAML::Key << "toolchains" << YAML::Value << YAML::BeginSeq;
    for (const auto& t : list) {
        e << YAML::BeginMap;
        e << YAML::Key << "version" << YAML::Value << t.version;
        e << YAML::Key << "path" << YAML::Value << t.path;
        e << YAML::Key << "platform" << YAML::Value << t.platform;
        e << YAML::EndMap;
    }
    e << YAML::EndSeq << YAML::EndMap;

    const FS::path final_path = ExeDir() / kLocalManifestName;
    const FS::path tmp_path = final_path.string() + ".tmp";

    {
        std::ofstream f(tmp_path, std::ios::trunc);
        if (!f) {
            std::cerr << "ce_griller: cannot write '" << tmp_path.string() << "'\n";
            return false;
        }
        f << e.c_str() << "\n";
        if (!f) return false;
    }

    std::error_code ec;
    FS::rename(tmp_path, final_path, ec);
    if (ec) {
        std::cerr << "ce_griller: cannot update toolchain manifest: " << ec.message() << "\n";
        return false;
    }
    return true;
}

// Newest installed toolchain for the given platform, if any.
std::optional<InstalledToolchain> NewestInstalled(const std::vector<InstalledToolchain>& list,
                                                  const std::string& platform) {
    std::optional<InstalledToolchain> best;
    for (const auto& t : list) {
        if (t.platform != platform) continue;
        if (!best || VersionLess(best->version, t.version)) best = t;
    }
    return best;
}

// Finds an exact version+platform and returns its directory.
bool ResolveToolchain(const std::vector<InstalledToolchain>& installed,
                      const std::string& version, const std::string& platform,
                      FS::path& dir) {
    const auto it = std::find_if(installed.begin(), installed.end(), [&](const auto& t) {
        return t.version == version && t.platform == platform;
    });
    if (it == installed.end()) {
        std::cerr << "ce_griller: toolchain " << version << " [" << platform
                  << "] is not installed. Run: ce_griller toolchain install "
                  << version << " --platform " << platform << "\n";
        return false;
    }

    dir = ExeDir() / it->path;
    if (!FS::is_directory(dir)) {
        std::cerr << "ce_griller: toolchain directory is missing: " << dir.string() << "\n";
        return false;
    }
    return true;
}

std::string ToolchainBinPath(const InstalledToolchain& t) {
    return (ExeDir() / t.path / kToolchainBinDir).lexically_normal().string();
}

// PATH always refers to the machine you're on, so this uses the host platform.
bool ToolchainPathAdd(const Options& options) {
    std::vector<InstalledToolchain> installed;
    if (!LoadInstalled(installed)) return false;

    const std::string platform = CurrentPlatform();
    std::optional<InstalledToolchain> pick;

    if (options.version == "latest") {
        pick = NewestInstalled(installed, platform);
    } else {
        for (const auto& t : installed)
            if (t.version == options.version && t.platform == platform) pick = t;
    }
    if (!pick) {
        std::cerr << "ce_griller: toolchain " << options.version << " [" << platform
                  << "] is not installed. Run: ce_griller toolchain install " << options.version << "\n";
        return false;
    }

    const std::string bin = ToolchainBinPath(*pick);
    if (!FS::is_directory(bin)) {
        std::cerr << "ce_griller: toolchain bin directory is missing: " << bin << "\n";
        return false;
    }

    // Only one ce_griller toolchain should be active at a time.
    for (const auto& t : installed) {
        if (t.platform != platform || t.version == pick->version) continue;
        Platforms::RemoveDirFromPath(ToolchainBinPath(t));
    }

    if (!Platforms::AddDirToPath(bin)) {
        std::cerr << "ce_griller: failed to update PATH\n";
        return false;
    }

    std::cout << "Added toolchain " << pick->version << " to your PATH:\n  " << bin << "\n"
              << "Restart your terminal for the change to take effect.\n";
    return true;
}

bool ToolchainPathRemove(const Options& options) {
    std::vector<InstalledToolchain> installed;
    if (!LoadInstalled(installed)) return false;

    const std::string platform = CurrentPlatform();
    const bool all = options.version == "all";
    bool matched = false, ok = true;

    for (const auto& t : installed) {
        if (t.platform != platform) continue;
        if (!all && t.version != options.version) continue;
        matched = true;
        if (!Platforms::RemoveDirFromPath(ToolchainBinPath(t))) {
            std::cerr << "ce_griller: failed to remove " << t.version << " from PATH\n";
            ok = false;
        } else {
            std::cout << "Removed toolchain " << t.version << " from your PATH.\n";
        }
    }
    if (!matched) {
        std::cerr << "ce_griller: toolchain " << options.version << " [" << platform << "] is not installed\n";
        return false;
    }
    return ok;
}

bool ToolchainPathStatus(const Options&) {
    std::vector<InstalledToolchain> installed;
    if (!LoadInstalled(installed)) return false;

    const std::string platform = CurrentPlatform();
    bool any = false;
    for (const auto& t : installed) {
        if (t.platform != platform) continue;
        any = true;
        std::cout << "  " << t.version << " [" << t.platform << "]  "
                  << (Platforms::IsDirInPath(ToolchainBinPath(t)) ? "on PATH" : "not on PATH") << "\n";
    }
    if (!any) std::cout << "No toolchains installed for " << platform << ".\n";
    return true;
}

//   manifest_version: 1
//   releases:
//     - version: 0.1.0
//       platform: windows-x64
//       url: https://github.com/.../ce_toolchain-0.1.0-windows-x64.zip

struct RemoteRelease {
    std::string version;
    std::string platform;
    std::string url;
};

// Auxiliary tool (e.g. the shader compiler) listed under "tools:" in the
// release manifest.
struct RemoteTool {
    std::string name;
    std::string version;
    std::string platform;
    std::string url;
    std::string executable;  // path of the executable inside the extracted archive
};

bool FetchReleases(std::vector<RemoteRelease>& out, std::vector<RemoteTool>* tools = nullptr) {
    out.clear();
    if (tools) tools->clear();

    std::string body;
    if (!HttpGetString(kReleaseManifestUrl, body)) return false;

    try {
        const YAML::Node manifest = YAML::Load(body);

        if (!manifest["manifest_version"] ||
            manifest["manifest_version"].as<uint32_t>() != kSupportedManifestVersion) {
            std::cerr << "ce_griller: unsupported release manifest version\n";
            return false;
        }

        const YAML::Node releases = manifest["releases"];
        if (!releases || !releases.IsSequence()) {
            std::cerr << "ce_griller: invalid release manifest\n";
            return false;
        }

        for (const YAML::Node& r : releases) {
            out.push_back({Required<std::string>(r, "version"),
                           Required<std::string>(r, "platform"),
                           Required<std::string>(r, "url")});
        }

        if (tools) {
            if (const YAML::Node t = manifest["tools"]) {
                if (!t.IsSequence()) {
                    std::cerr << "ce_griller: invalid 'tools' in release manifest\n";
                    return false;
                }
                for (const YAML::Node& x : t) {
                    tools->push_back({Required<std::string>(x, "name"),
                                      Required<std::string>(x, "version"),
                                      Required<std::string>(x, "platform"),
                                      Required<std::string>(x, "url"),
                                      Required<std::string>(x, "executable")});
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "ce_griller: failed to parse release manifest: " << e.what() << "\n";
        return false;
    }
}

std::optional<RemoteRelease> FindRelease(const std::vector<RemoteRelease>& releases,
                                         const std::string& version,
                                         const std::string& platform) {
    std::optional<RemoteRelease> best;

    for (const auto& r : releases) {
        if (r.platform != platform) continue;
        if (version == "latest") {
            if (!best || VersionLess(best->version, r.version)) best = r;
        } else if (r.version == version) {
            return r;
        }
    }
    return best;
}

bool InstallRelease(const RemoteRelease& release) {
    if (!IsSafeVersion(release.version)) {
        std::cerr << "ce_griller: refusing unsafe version string '" << release.version << "'\n";
        return false;
    }
    if (!IsSafeVersion(release.platform)) {
        std::cerr << "ce_griller: refusing unsafe platform string '" << release.platform << "'\n";
        return false;
    }
    if (release.url.rfind("https://", 0) != 0) {
        std::cerr << "ce_griller: refusing non-https download url\n";
        return false;
    }

    std::vector<InstalledToolchain> installed;
    if (!LoadInstalled(installed)) return false;

    // toolchains/<version>/<platform> so several platforms can coexist.
    const std::string rel_path =
        std::string(kToolchainDirName) + "/" + release.version + "/" + release.platform;
    const FS::path dest = ExeDir() / rel_path;
    const FS::path partial = dest.string() + ".partial";

    const bool listed = std::any_of(installed.begin(), installed.end(), [&](const auto& t) {
        return t.version == release.version && t.platform == release.platform;
    });
    if (listed && FS::is_directory(dest)) {
        std::cout << "Toolchain " << release.version << " [" << release.platform
                  << "] is already installed.\n";
        return true;
    }

    std::error_code ec;
    FS::remove_all(partial, ec);
    FS::remove_all(dest, ec);
    FS::create_directories(partial, ec);
    if (ec) {
        std::cerr << "ce_griller: cannot create '" << partial.string() << "': " << ec.message() << "\n";
        return false;
    }

    const FS::path archive_path =
        FS::temp_directory_path() /
        ("ce_toolchain_" + release.version + "_" + release.platform + ".download");

    std::cout << "Installing toolchain " << release.version
              << " [" << release.platform << "]\n";

    if (!HttpGetFile(release.url, archive_path)) {
        FS::remove(archive_path, ec);
        FS::remove_all(partial, ec);
        return false;
    }

    std::cout << "  extracting...\n";
    const bool extracted = ExtractArchive(archive_path, partial);
    FS::remove(archive_path, ec);

    if (!extracted) {
        FS::remove_all(partial, ec);
        return false;
    }

    FS::rename(partial, dest, ec);
    if (ec) {
        std::cerr << "ce_griller: cannot finalise install: " << ec.message() << "\n";
        FS::remove_all(partial, ec);
        return false;
    }

    installed.erase(std::remove_if(installed.begin(), installed.end(), [&](const auto& t) {
                        return t.version == release.version && t.platform == release.platform;
                    }),
                    installed.end());
    installed.push_back({release.version, rel_path, release.platform});

    if (!SaveInstalled(installed)) return false;

    std::cout << "Installed toolchain " << release.version
              << " [" << release.platform << "].\n";
    return true;
}

// Auxiliary tools (shader compiler etc.), downloaded on demand
struct InstalledTool {
    std::string name;
    std::string version;
    std::string platform;
    std::string path;        // relative to the executable directory
    std::string executable;  // relative to `path`
};

bool LoadTools(std::vector<InstalledTool>& out) {
    out.clear();
    const FS::path manifest_path = ExeDir() / kLocalToolsManifestName;
    if (!FS::exists(manifest_path)) return true;

    try {
        const YAML::Node m = YAML::LoadFile(manifest_path.string());

        if (!m["manifest_version"] ||
            m["manifest_version"].as<uint32_t>() != kSupportedManifestVersion) {
            std::cerr << "ce_griller: unsupported tools manifest version\n";
            return false;
        }

        const YAML::Node tools = m["tools"];
        if (!tools) return true;
        if (!tools.IsSequence()) {
            std::cerr << "ce_griller: invalid tools manifest\n";
            return false;
        }

        for (const YAML::Node& t : tools) {
            out.push_back({Required<std::string>(t, "name"),
                           Required<std::string>(t, "version"),
                           Required<std::string>(t, "platform"),
                           Required<std::string>(t, "path"),
                           Required<std::string>(t, "executable")});
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "ce_griller: failed to read tools manifest: " << e.what() << "\n";
        return false;
    }
}

bool SaveTools(const std::vector<InstalledTool>& list) {
    YAML::Emitter e;
    e << YAML::BeginMap;
    e << YAML::Key << "manifest_version" << YAML::Value << kSupportedManifestVersion;
    e << YAML::Key << "tools" << YAML::Value << YAML::BeginSeq;
    for (const auto& t : list) {
        e << YAML::BeginMap;
        e << YAML::Key << "name" << YAML::Value << t.name;
        e << YAML::Key << "version" << YAML::Value << t.version;
        e << YAML::Key << "platform" << YAML::Value << t.platform;
        e << YAML::Key << "path" << YAML::Value << t.path;
        e << YAML::Key << "executable" << YAML::Value << t.executable;
        e << YAML::EndMap;
    }
    e << YAML::EndSeq << YAML::EndMap;

    const FS::path final_path = ExeDir() / kLocalToolsManifestName;
    const FS::path tmp_path = final_path.string() + ".tmp";

    {
        std::ofstream f(tmp_path, std::ios::trunc);
        if (!f) {
            std::cerr << "ce_griller: cannot write '" << tmp_path.string() << "'\n";
            return false;
        }
        f << e.c_str() << "\n";
        if (!f) return false;
    }

    std::error_code ec;
    FS::rename(tmp_path, final_path, ec);
    if (ec) {
        std::cerr << "ce_griller: cannot update tools manifest: " << ec.message() << "\n";
        return false;
    }
    return true;
}

bool IsSafeRelativePath(const std::string& s) {
    if (s.empty()) return false;
    const FS::path p(s);
    if (p.is_absolute() || p.has_root_name() || p.has_root_directory()) return false;
    for (const auto& c : p) {
        if (c.string() == "..") return false;
    }
    return true;
}

// Downloads `url` and extracts it into `dest` (replacing whatever was there).
bool DownloadAndExtract(const std::string& url, const FS::path& dest, const std::string& tag) {
    const FS::path partial = dest.string() + ".partial";

    std::error_code ec;
    FS::remove_all(partial, ec);
    FS::remove_all(dest, ec);
    FS::create_directories(partial, ec);
    if (ec) {
        std::cerr << "ce_griller: cannot create '" << partial.string() << "': " << ec.message() << "\n";
        return false;
    }

    const FS::path archive_path = FS::temp_directory_path() / ("ce_" + tag + ".download");

    if (!HttpGetFile(url, archive_path)) {
        FS::remove(archive_path, ec);
        FS::remove_all(partial, ec);
        return false;
    }

    std::cout << "  extracting...\n";
    const bool extracted = ExtractArchive(archive_path, partial);
    FS::remove(archive_path, ec);

    if (!extracted) {
        FS::remove_all(partial, ec);
        return false;
    }

    FS::rename(partial, dest, ec);
    if (ec) {
        std::cerr << "ce_griller: cannot finalise install: " << ec.message() << "\n";
        FS::remove_all(partial, ec);
        return false;
    }
    return true;
}

bool InstallTool(const RemoteTool& t, std::vector<InstalledTool>& tools) {
    if (!IsSafeVersion(t.name) || !IsSafeVersion(t.version) || !IsSafeVersion(t.platform)) {
        std::cerr << "ce_griller: refusing unsafe tool name/version/platform\n";
        return false;
    }
    if (t.url.rfind("https://", 0) != 0) {
        std::cerr << "ce_griller: refusing non-https download url\n";
        return false;
    }
    if (!IsSafeRelativePath(t.executable)) {
        std::cerr << "ce_griller: refusing unsafe executable path '" << t.executable << "'\n";
        return false;
    }

    const std::string rel_path =
        std::string(kToolsDirName) + "/" + t.name + "/" + t.version + "/" + t.platform;
    const FS::path dest = ExeDir() / rel_path;

    std::cout << "Installing tool " << t.name << " " << t.version
              << " [" << t.platform << "]\n";

    if (!DownloadAndExtract(t.url, dest, t.name + "_" + t.version + "_" + t.platform)) return false;

    const FS::path exe = dest / t.executable;
    if (!FS::is_regular_file(exe)) {
        std::cerr << "ce_griller: '" << t.executable << "' not found in downloaded archive\n";
        std::error_code ec;
        FS::remove_all(dest, ec);
        return false;
    }

#ifndef _WIN32
    {
        std::error_code ec;
        FS::permissions(exe,
                        FS::perms::owner_exec | FS::perms::group_exec | FS::perms::others_exec,
                        FS::perm_options::add, ec);
    }
#endif

    tools.erase(std::remove_if(tools.begin(), tools.end(), [&](const auto& x) {
                    return x.name == t.name && x.version == t.version && x.platform == t.platform;
                }),
                tools.end());
    tools.push_back({t.name, t.version, t.platform, rel_path, t.executable});

    if (!SaveTools(tools)) return false;

    std::cout << "Installed tool " << t.name << " " << t.version << ".\n";
    return true;
}

// Returns the path of a runnable tool for the *host* platform, downloading it
// first if it isn't installed. `version` may be "latest".
bool EnsureTool(const std::string& name, const std::string& version, FS::path& exe_out) {
    std::vector<InstalledTool> tools;
    if (!LoadTools(tools)) return false;

    const std::string platform = CurrentPlatform();

    const InstalledTool* best = nullptr;
    for (const auto& t : tools) {
        if (t.name != name || t.platform != platform) continue;
        if (version != "latest" && t.version != version) continue;
        if (!FS::is_regular_file(ExeDir() / t.path / t.executable)) continue;
        if (!best || VersionLess(best->version, t.version)) best = &t;
    }
    if (best) {
        exe_out = ExeDir() / best->path / best->executable;
        return true;
    }

    std::cout << "Tool '" << name << "' is not installed, downloading...\n";

    std::vector<RemoteRelease> releases;
    std::vector<RemoteTool> remote;
    if (!FetchReleases(releases, &remote)) return false;

    std::optional<RemoteTool> pick;
    for (const auto& r : remote) {
        if (r.name != name || r.platform != platform) continue;
        if (version == "latest") {
            if (!pick || VersionLess(pick->version, r.version)) pick = r;
        } else if (r.version == version) {
            pick = r;
            break;
        }
    }
    if (!pick) {
        std::cerr << "ce_griller: no '" << name << "' (" << version << ") listed for "
                  << platform << " in the release manifest\n";
        return false;
    }

    if (!InstallTool(*pick, tools)) return false;

    exe_out = ExeDir() / kToolsDirName / pick->name / pick->version /
              pick->platform / pick->executable;
    return true;
}

// Project manifest
//   manifest_version: 1
//   toolchain: 0.1.0
//   build_output: build
//   game_data_file: data.tcf
//   script_dir: scripts
//   assets: [assets]
//   exclude: [".DS_Store", "*.psd"]
//   gameinfo:
//     game_name: ...   (see ParseGameInfo for all keys)
CE::Common::WindowMode ParseWindowMode(const std::string& text) {
    const std::string s = ToLower(text);
    if (s == "fullscreen") return CE::Common::WindowMode::Fullscreen;
    if (s == "borderless") return CE::Common::WindowMode::Borderless;
    if (s == "windowed") return CE::Common::WindowMode::Windowed;
    throw std::runtime_error("window_mode must be fullscreen, borderless or windowed");
}

const char* WindowModeName(CE::Common::WindowMode mode) {
    switch (mode) {
        case CE::Common::WindowMode::Fullscreen: return "fullscreen";
        case CE::Common::WindowMode::Borderless: return "borderless";
        default: return "windowed";
    }
}

CE::GameInfo ParseGameInfo(const YAML::Node& n) {
    CE::GameInfo g{};

    g.game_name = Required<std::string>(n, "game_name");
    g.game_version = Required<std::string>(n, "game_version");
    g.game_package_id = Required<std::string>(n, "package_id");
    g.startupFileName = Required<std::string>(n, "startup_file");

    g.windowWidth = Required<int>(n, "window_width");
    g.windowHeight = Required<int>(n, "window_height");
    g.windowTitle = Required<std::string>(n, "window_title");
    g.maxFPS = Required<int>(n, "max_fps");
    g.rendererName = Required<std::string>(n, "renderer");
    g.enableVSync = Required<bool>(n, "vsync");
    g.windowMode = ParseWindowMode(Required<std::string>(n, "window_mode"));
    g.resizableWindow = Required<bool>(n, "resizable");

    g.windowIcon = Optional<std::string>(n, "window_icon", "");
    g.pauseRenderingWhenFocusLostInWindowedMode = Optional<bool>(n, "no_render_on_focus_lost", false);
    g.pauseUpdateWhenFocusLost = Optional<bool>(n, "no_update_on_focus_lost", false);
    g.minWindowWidth = Optional<int>(n, "min_window_width", 0);
    g.minWindowHeight = Optional<int>(n, "min_window_height", 0);
    g.maxWindowWidth = Optional<int>(n, "max_window_width", 0);
    g.maxWindowHeight = Optional<int>(n, "max_window_height", 0);

    return g;
}

// Returns an empty string if valid, otherwise a description of the problem.
std::string ValidateGameInfo(const CE::GameInfo& g) {
    const std::string& id = g.game_package_id;
    if (id.empty() || id.front() == '.' || id.back() == '.' ||
        id.find("..") != std::string::npos) {
        return "package_id must not be empty, start/end with '.', or contain '..'";
    }
    for (unsigned char c : id) {
        if (!(std::isalnum(c) || c == '.' || c == '_' || c == '-')) {
            return "package_id may only contain ASCII letters, digits, '.', '_' and '-'";
        }
    }

    if (g.game_name.empty()) return "game_name must not be empty";
    if (g.game_version.empty()) return "game_version must not be empty";
    if (g.startupFileName.empty()) return "startup_file must not be empty";
    if (g.windowWidth <= 0 || g.windowHeight <= 0) return "window size must be positive";
    if (g.maxFPS <= 0) return "max_fps must be positive";

    const std::string r = ToLower(g.rendererName);
    if (r != "vulkan" && r != "software") return "renderer must be Vulkan or Software";

    if (g.minWindowWidth < 0 || g.minWindowHeight < 0 ||
        g.maxWindowWidth < 0 || g.maxWindowHeight < 0) {
        return "min/max window sizes must not be negative";
    }
    if (g.maxWindowWidth && g.minWindowWidth > g.maxWindowWidth) return "min_window_width > max_window_width";
    if (g.maxWindowHeight && g.minWindowHeight > g.maxWindowHeight) return "min_window_height > max_window_height";

    return {};
}

bool WriteGameinfoTxt(const CE::GameInfo& g, const FS::path& path) {
    std::ofstream f(path, std::ios::trunc);
    if (!f) return false;

    const auto b = [](bool v) { return v ? "true" : "false"; };

    f << "[Gameinfo]\n"
      << "Game_Name = " << g.game_name << "\n"
      << "Game_Version = " << g.game_version << "\n"
      << "Game_PackageID = " << g.game_package_id << "\n"
      << "Scripting_Startup_File = " << g.startupFileName << "\n";
    if (!g.windowIcon.empty()) f << "Window_Icon = " << g.windowIcon << "\n";

    f << "\n[Graphics]\n"
      << "Window_Width = " << g.windowWidth << "\n"
      << "Window_Height = " << g.windowHeight << "\n"
      << "Window_Title = " << g.windowTitle << "\n"
      << "Max_FPS = " << g.maxFPS << "\n"
      << "Renderer = " << g.rendererName << "\n"
      << "Enable_VSync = " << b(g.enableVSync) << "\n"
      << "Window_Mode = " << static_cast<int>(g.windowMode) << "\n"
      << "Resizable_Window = " << b(g.resizableWindow) << "\n"
      << "\n"
      << "No_Render_On_Focus_Lost = " << b(g.pauseRenderingWhenFocusLostInWindowedMode) << "\n"
      << "No_Update_On_Focus_Lost = " << b(g.pauseUpdateWhenFocusLost) << "\n";

    if (g.minWindowWidth > 0) f << "Min_Window_Width = " << g.minWindowWidth << "\n";
    if (g.minWindowHeight > 0) f << "Min_Window_Height = " << g.minWindowHeight << "\n";
    if (g.maxWindowWidth > 0) f << "Max_Window_Width = " << g.maxWindowWidth << "\n";
    if (g.maxWindowHeight > 0) f << "Max_Window_Height = " << g.maxWindowHeight << "\n";

    return static_cast<bool>(f);
}

bool LoadProject(const FS::path& root, CEBuildInfo& info) {
    const FS::path manifest_path = root / kProjectManifestName;
    if (!FS::exists(manifest_path)) {
        std::cerr << "ce_griller: no " << kProjectManifestName << " in '" << root.string() << "'\n";
        return false;
    }

    try {
        const YAML::Node m = YAML::LoadFile(manifest_path.string());

        info.manifest_version = Required<uint32_t>(m, "manifest_version");
        if (info.manifest_version != kSupportedProjectVersion) {
            std::cerr << "ce_griller: unsupported project manifest version "
                      << info.manifest_version << "\n";
            return false;
        }

        info.toolchain_needed = Required<std::string>(m, "toolchain");
        info.build_output = Optional<std::string>(m, "build_output", "build");
        info.game_data_file_name = Optional<std::string>(m, "game_data_file", "data.tcf");
        info.script_dir = Optional<std::string>(m, "script_dir", "scripts");

        info.shaders = ShaderConfig{};
        if (const YAML::Node sh = m["shaders"]) {
            if (!sh.IsMap()) throw std::runtime_error("'shaders' must be a map");
            info.shaders.source_dir = Optional<std::string>(sh, "source_dir", "shaders");
            info.shaders.compiler = Optional<std::string>(sh, "compiler", "glslc");
            info.shaders.compiler_version = Optional<std::string>(sh, "compiler_version", "latest");
            info.shaders.target_env = Optional<std::string>(sh, "target_env", "");
            if (const YAML::Node inc = sh["include"]) {
                for (const YAML::Node& p : inc) info.shaders.include_dirs.emplace_back(p.as<std::string>());
            }
        }
        if (!IsSafeVersion(info.shaders.compiler)) {
            throw std::runtime_error("shaders.compiler must be a plain tool name");
        }
        if (info.shaders.compiler_version != "latest" && !IsSafeVersion(info.shaders.compiler_version)) {
            throw std::runtime_error("shaders.compiler_version is not a valid version");
        }

        info.asset_paths.clear();
        if (const YAML::Node a = m["assets"]) {
            for (const YAML::Node& p : a) info.asset_paths.emplace_back(p.as<std::string>());
        } else {
            info.asset_paths.emplace_back("assets");
        }

        info.exclude_files.clear();
        if (const YAML::Node e = m["exclude"]) {
            for (const YAML::Node& p : e) info.exclude_files.push_back(p.as<std::string>());
        }

        const YAML::Node gi = m["gameinfo"];
        if (!gi || !gi.IsMap()) throw std::runtime_error("missing 'gameinfo' section");
        info.gameinfo = ParseGameInfo(gi);
        info.gameinfo.dataFileName = info.game_data_file_name;

        const FS::path data_name(info.game_data_file_name);
        if (data_name.empty() || data_name.filename() != data_name) {
            throw std::runtime_error("game_data_file must be a plain file name");
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "ce_griller: " << manifest_path.string() << ": " << e.what() << "\n";
        return false;
    }
}

bool IsExcluded(const FS::path& rel, const std::vector<std::string>& excludes) {
    const std::string generic = rel.generic_string();
    for (const std::string& e : excludes) {
        if (generic == e) return true;
        if (e.size() > 1 && e.front() == '*' && rel.extension() == e.substr(1)) return true;
        for (const auto& component : rel) {
            if (component.string() == e) return true;
        }
    }
    return false;
}

// Script sources never go into the data file (only compiled code.bin does).
bool IsScriptSource(const FS::path& p) {
    const std::string ext = ToLower(p.extension().string());
    return ext == ".as" || ext == ".ceas";
}

// Build steps

#ifdef _WIN32
constexpr const char* kHostExeExt = ".exe";
#else
constexpr const char* kHostExeExt = "";
#endif

// Build tools always run on the machine doing the build, so these come from
// the *host* toolchain, never the target one.
FS::path HostTool(const FS::path& host_toolchain, const std::string& name) {
    return host_toolchain / kToolchainBinDir / (name + kHostExeExt);
}

// Contents of each asset path land at the root of the data file, so
// assets/images/icon.png is addressed as "images/icon.png".
// Script sources (*.as, *.ceas) are skipped; they're compiled separately.
bool StageAssets(const FS::path& project_root, const CEBuildInfo& info, const FS::path& staging) {
    std::error_code ec;

    for (const FS::path& asset : info.asset_paths) {
        const FS::path src = project_root / asset;

        if (!FS::exists(src)) {
            std::cerr << "ce_griller: asset path '" << asset.string() << "' does not exist\n";
            return false;
        }

        if (FS::is_regular_file(src)) {
            if (IsExcluded(src.filename(), info.exclude_files) || IsScriptSource(src)) continue;
            FS::copy_file(src, staging / src.filename(), FS::copy_options::overwrite_existing, ec);
            if (ec) {
                std::cerr << "ce_griller: copy failed: " << ec.message() << "\n";
                return false;
            }
            continue;
        }

        for (FS::recursive_directory_iterator it(src), end; it != end; ++it) {
            const FS::path rel = FS::relative(it->path(), src);

            if (IsExcluded(rel, info.exclude_files)) {
                if (it->is_directory()) it.disable_recursion_pending();
                continue;
            }

            const FS::path target = staging / rel;
            if (it->is_directory()) {
                FS::create_directories(target, ec);
            } else if (it->is_regular_file()) {
                if (IsScriptSource(it->path())) continue;
                FS::create_directories(target.parent_path(), ec);
                FS::copy_file(it->path(), target, FS::copy_options::overwrite_existing, ec);
            }
            if (ec) {
                std::cerr << "ce_griller: copy failed: " << ec.message() << "\n";
                return false;
            }
        }
    }
    return true;
}

// Copies the toolchain's precompiled SPIR-V shaders into the staging area,
// if the toolchain ships any. (SPIR-V is platform independent, but we take it
// from the target toolchain so it matches the runtime being shipped.)
bool StageShaders(const FS::path& target_toolchain, const FS::path& staging, bool verbose) {
    const FS::path src = target_toolchain / kToolchainShaderDir;
    if (!FS::is_directory(src)) {
        if (verbose) std::cout << "  (no precompiled shaders in toolchain, skipping)\n";
        return true;
    }

    const FS::path dst = staging / kToolchainShaderDir;
    std::error_code ec;
    FS::remove_all(dst, ec);
    FS::create_directories(dst, ec);
    FS::copy(src, dst, FS::copy_options::recursive | FS::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "ce_griller: failed to stage shaders: " << ec.message() << "\n";
        return false;
    }
    return true;
}

bool IsShaderSource(const FS::path& p) {
    static const char* const kExts[] = {".vert", ".frag", ".comp", ".geom",
                                        ".tesc", ".tese", ".mesh", ".task"};
    const std::string ext = ToLower(p.extension().string());
    return std::any_of(std::begin(kExts), std::end(kExts),
                       [&](const char* e) { return ext == e; });
}

// Compiles <shaders.source_dir>/**/*.{vert,frag,comp,...} to SPIR-V in
// staging/shaders/Compiled/SPIRV/<relative path>.spv. Downloads the compiler
// the first time it's needed. Does nothing if there are no shader sources.
bool CompileUserShaders(const FS::path& project_root, const CEBuildInfo& info,
                        const FS::path& staging, bool debug, bool verbose) {
    const FS::path src_dir = project_root / info.shaders.source_dir;
    if (!FS::is_directory(src_dir)) return true;

    std::vector<FS::path> sources;
    std::error_code ec;
    for (FS::recursive_directory_iterator it(src_dir, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file() && IsShaderSource(it->path())) sources.push_back(it->path());
    }
    if (ec) {
        std::cerr << "ce_griller: failed to scan shaders: " << ec.message() << "\n";
        return false;
    }
    if (sources.empty()) return true;
    std::sort(sources.begin(), sources.end());

    FS::path compiler;
    if (!EnsureTool(info.shaders.compiler, info.shaders.compiler_version, compiler)) return false;

    const bool glslang = ToLower(info.shaders.compiler).find("glslang") != std::string::npos;

    std::vector<std::string> base = {compiler.string()};
    if (glslang) {
        base.emplace_back("-V");
        if (debug) { base.emplace_back("-g"); base.emplace_back("-Od"); }
        if (!info.shaders.target_env.empty()) {
            base.emplace_back("--target-env");
            base.push_back(info.shaders.target_env);
        }
    } else {  // glslc
        if (debug) { base.emplace_back("-g"); base.emplace_back("-O0"); }
        else base.emplace_back("-O");
        if (!info.shaders.target_env.empty()) {
            base.push_back("--target-env=" + info.shaders.target_env);
        }
    }

    std::vector<FS::path> includes = {src_dir};
    for (const FS::path& inc : info.shaders.include_dirs) includes.push_back(project_root / inc);
    for (const FS::path& inc : includes) {
        if (glslang) {
            base.push_back("-I" + inc.string());
        } else {
            base.emplace_back("-I");
            base.push_back(inc.string());
        }
    }

    std::cout << "Compiling " << sources.size() << " shader(s) with "
              << info.shaders.compiler << (debug ? " (debug)" : "") << "...\n";

    for (const FS::path& src : sources) {
        FS::path out = staging / kToolchainShaderDir / FS::relative(src, src_dir);
        out += ".spv";
        FS::create_directories(out.parent_path(), ec);

        std::vector<std::string> args = base;
        args.push_back(src.string());
        args.emplace_back("-o");
        args.push_back(out.string());

        if (!RunProcess(args, verbose)) {
            std::cerr << "ce_griller: shader compilation failed: " << src.string() << "\n";
            return false;
        }
    }
    return true;
}

// ce_scriptc compile <startup> -o <staging>/code.bin -I <script_dir> [-g]
bool CompileScripts(const FS::path& host_toolchain, const FS::path& project_root,
                    const CEBuildInfo& info, const FS::path& staging,
                    bool debug, bool verbose) {
    const FS::path scriptc = HostTool(host_toolchain, "ce_scriptc");
    if (!FS::exists(scriptc)) {
        std::cerr << "ce_griller: script compiler not found: " << scriptc.string() << "\n";
        return false;
    }

    const FS::path script_dir = project_root / info.script_dir;
    const FS::path startup(info.gameinfo.startupFileName);
    const FS::path input = startup.is_absolute() ? startup : script_dir / startup;

    if (!FS::exists(input)) {
        std::cerr << "ce_griller: startup script does not exist: " << input.string() << "\n";
        return false;
    }

    const FS::path output = staging / "code.bin";

    std::cout << "Compiling script: " << input.string() << (debug ? " (debug)" : "") << "\n";

    std::vector<std::string> args = {
        scriptc.string(), "compile", input.string(),
        "-o", output.string(),
        "-I", script_dir.string(),
    };
    if (debug) args.emplace_back("-g");

    if (!RunProcess(args, verbose)) {
        std::cerr << "ce_griller: script compilation failed: " << input.string() << "\n";
        return false;
    }
    return true;
}

// ce_tcf pack <staging> <output>
bool PackDataFile(const FS::path& host_toolchain, const FS::path& staging,
                  const FS::path& output_file, bool verbose) {
    const FS::path tcf = HostTool(host_toolchain, "ce_tcf");
    if (!FS::exists(tcf)) {
        std::cerr << "ce_griller: tcf tool not found: " << tcf.string() << "\n";
        return false;
    }

    std::error_code ec;
    FS::create_directories(output_file.parent_path(), ec);

    std::cout << "Creating TCF archive: " << output_file.string() << "\n";

    if (!RunProcess({tcf.string(), "pack", staging.string(), output_file.string()}, verbose)) {
        std::cerr << "ce_griller: TCF archive creation failed: " << output_file.string() << "\n";
        return false;
    }
    return true;
}

const char* SharedLibExt(const std::string& platform) {
    if (platform.rfind("windows", 0) == 0) return ".dll";
    if (platform.rfind("macos", 0) == 0) return ".dylib";
    return ".so";
}

// Looks for <stem><ext>
std::optional<FS::path> FindSharedLib(const FS::path& dir, const std::string& stem,
                                      const std::string& platform) {
    const FS::path p = dir / (stem + SharedLibExt(platform));
    if (FS::is_regular_file(p)) return p;
    return std::nullopt;
}

const char* ExeExt(const std::string& platform) {
    return platform.rfind("windows", 0) == 0 ? ".exe" : "";
}

// Puts the loader executable plus ce_runtime and ce_common for the *target*
// platform into the build output.
bool CopyRuntimeFiles(const FS::path& target_toolchain, const std::string& target_platform,
                      const FS::path& out_dir) {
    const FS::path runtime_dir = target_toolchain / kToolchainRuntimeDir;
    std::error_code ec;

    // Loader executable (ce_loader.exe / ce_loader)
    const std::string loader_name = std::string("ce_loader") + ExeExt(target_platform);
    const FS::path loader = runtime_dir / loader_name;
    if (!FS::is_regular_file(loader)) {
        std::cerr << "ce_griller: " << loader_name << " not found in " << runtime_dir.string() << "\n";
        return false;
    }
    FS::copy_file(loader, out_dir / loader_name, FS::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "ce_griller: failed to copy " << loader_name << ": " << ec.message() << "\n";
        return false;
    }
    FS::permissions(out_dir / loader_name,
                    FS::perms::owner_exec | FS::perms::group_exec | FS::perms::others_exec,
                    FS::perm_options::add, ec);
    std::cout << "  copied " << loader_name << "\n";

    // Shared libraries
    for (const char* stem : {"ce_runtime", "ce_common"}) {
        const auto lib = FindSharedLib(runtime_dir, stem, target_platform);
        if (!lib) {
            std::cerr << "ce_griller: " << stem << SharedLibExt(target_platform)
                      << " not found in " << runtime_dir.string() << "\n";
            return false;
        }

        FS::copy_file(*lib, out_dir / lib->filename(), FS::copy_options::overwrite_existing, ec);
        if (ec) {
            std::cerr << "ce_griller: failed to copy " << lib->filename().string()
                      << ": " << ec.message() << "\n";
            return false;
        }
        std::cout << "  copied " << lib->filename().string() << "\n";
    }
    return true;
}

}  // namespace

// Commands
bool CreateNewProject(const Options& options) {
    try {
        const FS::path root = options.project_root;

        if (FS::exists(root)) {
            if (!FS::is_directory(root) || !FS::is_empty(root)) {
                std::cerr << "ce_griller: '" << root.string() << "' exists and is not an empty directory\n";
                return false;
            }
        }

        FS::path abs = FS::absolute(root).lexically_normal();
        if (!abs.has_filename()) abs = abs.parent_path();
        const std::string name = abs.filename().string();

        std::string id_part;
        for (unsigned char c : name) {
            if (std::isalnum(c)) id_part.push_back(static_cast<char>(std::tolower(c)));
        }
        if (id_part.empty()) id_part = "game";

        std::vector<InstalledToolchain> installed;
        LoadInstalled(installed);
        const auto newest = NewestInstalled(installed, CurrentPlatform());
        const std::string toolchain = newest ? newest->version : "";
        if (!newest) {
            std::cerr << "ce_griller: warning: no toolchain installed; set 'toolchain' in "
                      << kProjectManifestName << " (try: ce_griller toolchain install latest)\n";
        }

        FS::create_directories(root / "assets");
        FS::create_directories(root / "scripts");
        FS::create_directories(root / "shaders");

        YAML::Emitter e;
        e << YAML::BeginMap;
        e << YAML::Key << "manifest_version" << YAML::Value << kSupportedProjectVersion;
        e << YAML::Key << "toolchain" << YAML::Value << toolchain;
        e << YAML::Key << "build_output" << YAML::Value << "build";
        e << YAML::Key << "game_data_file" << YAML::Value << "data.tcf";
        e << YAML::Key << "script_dir" << YAML::Value << "scripts";
        e << YAML::Key << "assets" << YAML::Value << YAML::BeginSeq << "assets" << YAML::EndSeq;
        e << YAML::Key << "exclude" << YAML::Value << YAML::BeginSeq << ".DS_Store" << YAML::EndSeq;
        e << YAML::Key << "shaders" << YAML::Value << YAML::BeginMap;
        e << YAML::Key << "source_dir" << YAML::Value << "shaders";
        e << YAML::Key << "compiler" << YAML::Value << "glslc";
        e << YAML::Key << "compiler_version" << YAML::Value << "latest";
        e << YAML::EndMap;
        e << YAML::Key << "gameinfo" << YAML::Value << YAML::BeginMap;
        e << YAML::Key << "game_name" << YAML::Value << name;
        e << YAML::Key << "game_version" << YAML::Value << "1.0.0";
        e << YAML::Key << "package_id" << YAML::Value << ("com.example." + id_part);
        e << YAML::Key << "startup_file" << YAML::Value << "startup.as";
        e << YAML::Key << "window_width" << YAML::Value << 1280;
        e << YAML::Key << "window_height" << YAML::Value << 720;
        e << YAML::Key << "window_title" << YAML::Value << name;
        e << YAML::Key << "max_fps" << YAML::Value << 144;
        e << YAML::Key << "renderer" << YAML::Value << "Vulkan";
        e << YAML::Key << "vsync" << YAML::Value << true;
        e << YAML::Key << "window_mode" << YAML::Value << WindowModeName(CE::Common::WindowMode::Windowed);
        e << YAML::Key << "resizable" << YAML::Value << true;
        e << YAML::Key << "no_render_on_focus_lost" << YAML::Value << true;
        e << YAML::Key << "no_update_on_focus_lost" << YAML::Value << true;
        e << YAML::Key << "min_window_width" << YAML::Value << 640;
        e << YAML::Key << "min_window_height" << YAML::Value << 360;
        e << YAML::EndMap << YAML::EndMap;

        std::ofstream f(root / kProjectManifestName, std::ios::trunc);
        if (!f) {
            std::cerr << "ce_griller: cannot write " << kProjectManifestName << "\n";
            return false;
        }
        f << e.c_str() << "\n";

        std::cout << "Created project '" << name << "' in " << root.string() << "\n";
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "ce_griller: " << ex.what() << "\n";
        return false;
    }
}

bool BuildProject(const Options& options) {
    try {
        const FS::path root = options.project_root;
        const bool verbose = options.debug;

        CEBuildInfo info;
        if (!LoadProject(root, info)) return false;

        if (const std::string problem = ValidateGameInfo(info.gameinfo); !problem.empty()) {
            std::cerr << "ce_griller: invalid gameinfo: " << problem << "\n";
            return false;
        }

        // Resolve toolchains: build tools come from the host toolchain, the
        // runtime libraries come from the target platform's toolchain.
        std::vector<InstalledToolchain> installed;
        if (!LoadInstalled(installed)) return false;

        const std::string host_platform = CurrentPlatform();
        const std::string target_platform = TargetPlatform(options);

        FS::path host_dir;
        if (!ResolveToolchain(installed, info.toolchain_needed, host_platform, host_dir)) return false;

        FS::path target_dir = host_dir;
        if (target_platform != host_platform) {
            if (!ResolveToolchain(installed, info.toolchain_needed, target_platform, target_dir)) return false;
        }

        const FS::path out_dir = root / info.build_output;
        const FS::path staging = out_dir / ".staging";

        std::error_code ec;
        FS::remove_all(staging, ec);
        FS::create_directories(staging, ec);
        if (ec) {
            std::cerr << "ce_griller: cannot create '" << staging.string() << "': " << ec.message() << "\n";
            return false;
        }

        std::cout << "Building for " << target_platform << " with toolchain "
                  << info.toolchain_needed << (options.debug ? " (debug)" : "") << "...\n";

        std::cout << "Staging assets...\n";
        if (!StageAssets(root, info, staging)) return false;
        if (!StageShaders(target_dir, staging, verbose)) return false;
        if (!CompileUserShaders(root, info, staging, options.debug, verbose)) return false;

        if (!WriteGameinfoTxt(info.gameinfo, staging / "Gameinfo.txt")) {
            std::cerr << "ce_griller: failed to write Gameinfo.txt\n";
            return false;
        }

        if (!CompileScripts(host_dir, root, info, staging, options.debug, verbose)) return false;
        if (!PackDataFile(host_dir, staging, out_dir / info.game_data_file_name, verbose)) return false;

        std::cout << "Copying runtime...\n";
        if (!CopyRuntimeFiles(target_dir, target_platform, out_dir)) return false;

        {
            std::ofstream cfg(out_dir / "path.cfg", std::ios::trunc);
            cfg << info.game_data_file_name << "\n";
            if (!cfg) {
                std::cerr << "ce_griller: failed to write path.cfg\n";
                return false;
            }
        }

        if (options.debug || options.keep_staging) {
            std::cout << "Staging kept at: " << staging.string() << "\n";
        } else {
            FS::remove_all(staging, ec);
        }
        std::cout << "Build complete: " << out_dir.string() << "\n";
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "ce_griller: build failed: " << ex.what() << "\n";
        return false;
    }
}

bool InstallToolchain(const Options& options) {
    std::vector<RemoteRelease> releases;
    if (!FetchReleases(releases)) return false;

    const std::string platform = TargetPlatform(options);
    const auto release = FindRelease(releases, options.version, platform);
    if (!release) {
        std::cerr << "ce_griller: no toolchain '" << options.version
                  << "' available for " << platform << "\n";
        return false;
    }
    return InstallRelease(*release);
}

bool UpdateToolchain(const Options& options) {
    std::vector<RemoteRelease> releases;
    if (!FetchReleases(releases)) return false;

    const std::string platform = TargetPlatform(options);
    const auto latest = FindRelease(releases, "latest", platform);
    if (!latest) {
        std::cerr << "ce_griller: no toolchains available for " << platform << "\n";
        return false;
    }

    std::vector<InstalledToolchain> installed;
    if (!LoadInstalled(installed)) return false;

    if (const auto newest = NewestInstalled(installed, platform);
        newest && !VersionLess(newest->version, latest->version) &&
        FS::is_directory(ExeDir() / newest->path)) {
        std::cout << "Already up to date (" << newest->version << " [" << platform << "]).\n";
        return true;
    }

    return InstallRelease(*latest);
}

bool ListToolchains(const Options&) {
    try {
        std::vector<InstalledToolchain> installed;
        if (!LoadInstalled(installed)) return false;

        std::vector<InstalledTool> tools;
        if (!LoadTools(tools)) return false;

        if (installed.empty()) {
            std::cout << "No toolchains installed.\n";
        } else {
            std::cout << "Installed toolchains:\n";
            for (const auto& t : installed) {
                const bool present = FS::is_directory(ExeDir() / t.path);
                std::cout << "  " << t.version << " [" << t.platform << "]"
                          << (present ? "" : " (missing)") << '\n';
            }
        }

        if (!tools.empty()) {
            std::cout << "Installed tools:\n";
            for (const auto& t : tools) {
                const bool present = FS::is_regular_file(ExeDir() / t.path / t.executable);
                std::cout << "  " << t.name << " " << t.version << " [" << t.platform << "]"
                          << (present ? "" : " (missing)") << '\n';
            }
        }
        return true;
    } catch (const FS::filesystem_error& e) {
        std::cerr << "ce_griller: filesystem error: " << e.what() << '\n';
        return false;
    }
}

int main(int argc, char* argv[]) {
    Options options;

    if (!ParseArguments(argc, argv, options)) {
        ShowHelp();
        return ToUint32(CEGrillerReturns::ArgumentParseFail);
    }

    if (options.show_help) {
        ShowHelp();
        return ToUint32(CEGrillerReturns::Okay);
    }

    if (options.show_version) {
        PrintVersion();
        return ToUint32(CEGrillerReturns::Okay);
    }

    CurlGlobal curl_guard;

    switch (options.command) {
        case CEGrillerCommand::New:
            if (!CreateNewProject(options)) {
                return ToUint32(CEGrillerReturns::CreateNewProjectFail);
            }
            break;

        case CEGrillerCommand::Build:
            if (!BuildProject(options)) {
                return ToUint32(CEGrillerReturns::BuildProjectFail);
            }
            break;

        case CEGrillerCommand::ToolchainInstall:
            if (!InstallToolchain(options)) {
                return ToUint32(CEGrillerReturns::ToolchainInstallFail);
            }
            break;

        case CEGrillerCommand::ToolchainUpdate:
            if (!UpdateToolchain(options)) {
                return ToUint32(CEGrillerReturns::ToolchainUpdateFail);
            }
            break;

        case CEGrillerCommand::ToolchainList:
            if (!ListToolchains(options)) {
                return ToUint32(CEGrillerReturns::ToolchainListFail);
            }
            break;

        case CEGrillerCommand::None:
            return ToUint32(CEGrillerReturns::ArgumentParseFail);

        case CEGrillerCommand::ToolchainPathAdd:
            if (!ToolchainPathAdd(options)) return ToUint32(CEGrillerReturns::ToolchainPathFail);
            break;
        case CEGrillerCommand::ToolchainPathRemove:
            if (!ToolchainPathRemove(options)) return ToUint32(CEGrillerReturns::ToolchainPathFail);
            break;
        case CEGrillerCommand::ToolchainPathStatus:
            if (!ToolchainPathStatus(options)) return ToUint32(CEGrillerReturns::ToolchainPathFail);
            break;
    }

    return ToUint32(CEGrillerReturns::Okay);
}