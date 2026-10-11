
#include <ce_common/tcf.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace TCF = CECommon::TCF;

namespace {
    constexpr size_t kBufferSize = 64 * 1024;

    void PrintHelp() {
        std::cout
            << "TCF Archive Tool\n"
            << "\n"
            << "Usage:\n"
            << "  tcf pack <directory> <archive.tcf> [compression]\n"
            << "  tcf unpack <archive.tcf> <output-directory>\n"
            << "  tcf extract <archive.tcf> <internal-file-path> <output-file>\n"
            << "  tcf tree <archive.tcf> [internal-directory-path]\n"
            << "  tcf help\n"
            << "\n"
            << "Commands:\n"
            << "  pack     Pack a directory recursively into a TCF archive.\n"
            << "  unpack   Extract the entire archive into a directory.\n"
            << "  extract  Extract one file from an archive.\n"
            << "  tree     Display an archive directory tree.\n"
            << "\n"
            << "Compression:\n"
            << "  zstd     Zstandard compression (default).\n"
            << "  lz4      LZ4 compression.\n"
            << "  none     No compression.\n";
    }

    std::string JoinArchivePath(const std::string& parent, const std::string& name) {
        if (parent.empty()) {
            return name;
        }

        return parent + "/" + name;
    }

    // Reject paths that could escape the output directory during unpacking.
    bool IsSafeRelativePath(const fs::path& path) {
        if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
            return false;
        }

        for (const auto& component : path) {
            if (component == ".." || component == "." || component.empty()) {
                return false;
            }
        }

        return true;
    }

    bool ExtractFile(
        TCF::TCFArchive& archive,
        const std::string& archivePath,
        const fs::path& outputPath
    ) {
        if (!archive.FileExists(archivePath)) {
            std::cerr << "File not found in archive: " << archivePath << '\n';
            return false;
        }

        TCF::TCFFile file = archive.OpenFile(archivePath);
        if (!file.IsValid()) {
            std::cerr << "Failed to open archive file: " << archivePath << '\n';
            return false;
        }

        const uint64_t fileSize = file.Size();

        std::error_code ec;
        if (!outputPath.parent_path().empty()) {
            fs::create_directories(outputPath.parent_path(), ec);
            if (ec) {
                std::cerr << "Failed to create output directory: "
                          << ec.message() << '\n';
                return false;
            }
        }

        std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
        if (!output) {
            std::cerr << "Failed to create output file: "
                      << outputPath.string() << '\n';
            return false;
        }

        std::vector<char> buffer(kBufferSize);
        uint64_t remaining = fileSize;

        while (remaining > 0) {
            const size_t amount = static_cast<size_t>(
                remaining < buffer.size() ? remaining : buffer.size()
            );

            if (!file.Read(buffer.data(), amount)) {
                std::cerr << "Failed to read archive file: " << archivePath << '\n';
                return false;
            }

            output.write(buffer.data(), static_cast<std::streamsize>(amount));
            if (!output) {
                std::cerr << "Failed to write output file: "
                          << outputPath.string() << '\n';
                return false;
            }

            remaining -= amount;
        }

        return true;
    }

    bool UnpackDirectory(
        TCF::TCFArchive& archive,
        const std::string& archiveDirectory,
        const fs::path& outputDirectory
    ) {
        std::error_code ec;
        fs::create_directories(outputDirectory, ec);

        if (ec) {
            std::cerr << "Failed to create directory: "
                      << outputDirectory.string() << ": "
                      << ec.message() << '\n';
            return false;
        }

        TCF::TCFDirectoryContents contents =
            archive.ListDirectory(archiveDirectory);

        for (const auto& directory : contents.directories) {
            const std::string childArchivePath =
                JoinArchivePath(archiveDirectory, directory.name);

            const fs::path relativePath(directory.name);
            if (!IsSafeRelativePath(relativePath)) {
                std::cerr << "Unsafe directory name in archive: "
                          << directory.name << '\n';
                return false;
            }

            if (!UnpackDirectory(
                    archive,
                    childArchivePath,
                    outputDirectory / relativePath
                )) {
                return false;
            }
        }

        for (const auto& file : contents.files) {
            const std::string childArchivePath =
                JoinArchivePath(archiveDirectory, file.name);

            const fs::path relativePath(file.name);
            if (!IsSafeRelativePath(relativePath)) {
                std::cerr << "Unsafe file name in archive: "
                          << file.name << '\n';
                return false;
            }

            if (!ExtractFile(
                    archive,
                    childArchivePath,
                    outputDirectory / relativePath
                )) {
                return false;
            }
        }

        return true;
    }

    void PrintTree(
        TCF::TCFArchive& archive,
        const std::string& archiveDirectory,
        const std::string& prefix,
        bool isLast,
        bool isRoot
    ) {
        if (!isRoot) {
            const size_t separator = archiveDirectory.find_last_of('/');
            const std::string name = separator == std::string::npos
                ? archiveDirectory
                : archiveDirectory.substr(separator + 1);

            std::cout << prefix
                      << (isLast ? "`-- " : "|-- ")
                      << name << "/\n";
        }

        const std::string childPrefix = isRoot
            ? ""
            : prefix + (isLast ? "    " : "|   ");

        TCF::TCFDirectoryContents contents =
            archive.ListDirectory(archiveDirectory);

        const size_t total = contents.directories.size() + contents.files.size();
        size_t index = 0;

        for (const auto& directory : contents.directories) {
            ++index;
            PrintTree(
                archive,
                JoinArchivePath(archiveDirectory, directory.name),
                childPrefix,
                index == total,
                false
            );
        }

        for (const auto& file : contents.files) {
            ++index;
            std::cout << childPrefix
                      << (index == total ? "`-- " : "|-- ")
                      << file.name << '\n';
        }
    }

    bool PackDirectory(
        const fs::path& inputDirectory,
        const fs::path& archivePath,
        const std::string& compression
    ) {
        if (!fs::exists(inputDirectory) ||
            !fs::is_directory(inputDirectory)) {
            std::cerr << "Input is not a directory: "
                      << inputDirectory.string() << '\n';
            return false;
        }

        TCF::TCFWriter writer(archivePath.string());

        if (compression == "zstd") {
            writer.SetCompression(TCF::TCFArchive::CompressionType::Zstd);
        } else if (compression == "lz4") {
            writer.SetCompression(TCF::TCFArchive::CompressionType::LZ4);
        } else if (compression == "none") {
            writer.SetCompression(TCF::TCFArchive::CompressionType::None);
        } else {
            std::cerr << "Unknown compression type: " << compression << '\n';
            return false;
        }

        if (!writer.AddDirectoryFromDisk("", inputDirectory)) {
            std::cerr << "Failed to pack directory: "
                      << inputDirectory.string() << '\n';
            return false;
        }

        if (!writer.Finish()) {
            std::cerr << "Failed to finish archive: "
                      << archivePath.string() << '\n';
            return false;
        }

        std::cout << "Packed " << inputDirectory.string()
                  << " -> " << archivePath.string() << '\n';
        return true;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintHelp();
        return 1;
    }

    const std::string command = argv[1];

    try {
        if (command == "help" || command == "--help" || command == "-h") {
            PrintHelp();
            return 0;
        }

        if (command == "pack") {
            if (argc < 4 || argc > 5) {
                std::cerr << "Usage: tcf pack <directory> <archive.tcf> [compression]\n";
                return 1;
            }

            const std::string compression = argc == 5 ? argv[4] : "zstd";

            return PackDirectory(argv[2], argv[3], compression) ? 0 : 1;
        }

        if (command == "unpack") {
            if (argc != 4) {
                std::cerr << "Usage: tcf unpack <archive.tcf> <output-directory>\n";
                return 1;
            }

            TCF::TCFArchive archive(argv[2]);

            if (!UnpackDirectory(archive, "", fs::path(argv[3]))) {
                return 1;
            }

            std::cout << "Archive unpacked to " << argv[3] << '\n';
            return 0;
        }

        if (command == "extract") {
            if (argc != 5) {
                std::cerr << "Usage: tcf extract <archive.tcf> <internal-file-path> <output-file>\n";
                return 1;
            }

            TCF::TCFArchive archive(argv[2]);

            return ExtractFile(archive, argv[3], fs::path(argv[4])) ? 0 : 1;
        }

        if (command == "tree") {
            if (argc < 3 || argc > 4) {
                std::cerr << "Usage: tcf tree <archive.tcf> [internal-directory-path]\n";
                return 1;
            }

            TCF::TCFArchive archive(argv[2]);
            const std::string directory = argc == 4 ? argv[3] : "";

            if (!archive.DirExists(directory)) {
                std::cerr << "Directory not found in archive: "
                          << directory << '\n';
                return 1;
            }

            std::cout << (directory.empty() ? "/" : directory + "/") << '\n';
            PrintTree(archive, directory, "", true, true);
            return 0;
        }

        std::cerr << "Unknown command: " << command << '\n';
        PrintHelp();
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}