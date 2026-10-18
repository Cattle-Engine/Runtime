#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "ce_common/as_compiler/bytecode.hpp" 

using namespace CE::Scripting::Impl;

namespace {
    const std::string& SymInternal(const GeneratedSymbolInfo& s)  { return s.InternalName; }
    const std::string& SymQualified(const GeneratedSymbolInfo& s) { return s.QualifiedName; }
    const std::string& SymDisplay(const GeneratedSymbolInfo& s)   { return s.DisplayName; }
    const std::string& SymKind(const GeneratedSymbolInfo& s)      { return s.Kind; }

    const char* ErrorText(int code) {
        switch (code) {
            case ByteCodeFormat::ToCode(ByteCodeFormat::ByteCodeError::None):               return "no error";
            case ByteCodeFormat::ToCode(ByteCodeFormat::ByteCodeError::Io):                 return "I/O error";
            case ByteCodeFormat::ToCode(ByteCodeFormat::ByteCodeError::BadMagic):           return "not a CEBYC file (bad magic)";
            case ByteCodeFormat::ToCode(ByteCodeFormat::ByteCodeError::UnsupportedVersion): return "unsupported CEBYC version";
            case ByteCodeFormat::ToCode(ByteCodeFormat::ByteCodeError::Corrupt):            return "file is corrupt";
            default:                                                                         return "unknown error";
        }
    }

    void Usage(const char* argv0) {
        std::cerr << "Usage: " << argv0 << " [-s|--sections] [-e|--entry NAME]... [-n|--names-only] file.cebyc\n";
    }

    void PrintSymbols(const std::vector<GeneratedSymbolInfo>& symbols) {
        size_t w_kind = 4, w_internal = 8, w_qual = 9;
        for (const auto& s : symbols) {
            w_kind     = std::max(w_kind, SymKind(s).size());
            w_internal = std::max(w_internal, SymInternal(s).size());
            w_qual     = std::max(w_qual, SymQualified(s).size());
        }

        std::cout << "Symbol table contains " << symbols.size() << " entries:\n";
        if (symbols.empty()) {
            return;
        }

        std::cout << std::setw(6) << "Num:" << "  "
                  << std::left << std::setw(static_cast<int>(w_kind)) << "Kind" << "  "
                  << std::setw(static_cast<int>(w_internal)) << "Internal" << "  "
                  << std::setw(static_cast<int>(w_qual)) << "Qualified" << "  "
                  << "Display\n" << std::right;

        for (size_t i = 0; i < symbols.size(); ++i) {
            const auto& s = symbols[i];
            std::cout << std::setw(5) << i << ":" << "  "
                      << std::left << std::setw(static_cast<int>(w_kind)) << SymKind(s) << "  "
                      << std::setw(static_cast<int>(w_internal)) << SymInternal(s) << "  "
                      << std::setw(static_cast<int>(w_qual)) << SymQualified(s) << "  "
                      << SymDisplay(s) << '\n' << std::right;
        }
    }

    void PrintSections(const std::vector<ScriptSectionInfo>& sections) {
        std::cout << "\nScript section table contains " << sections.size() << " entries:\n";
        if (sections.empty()) {
            return;
        }

        auto size_str = [](const std::optional<std::string>& v) {
            return v ? std::to_string(v->size()) : std::string("-");
        };

        size_t w_name = 4;
        for (const auto& s : sections) {
            w_name = std::max(w_name, s.Name.size());
        }

        std::cout << std::setw(6) << "Num:" << "  "
                  << std::left << std::setw(static_cast<int>(w_name)) << "Name" << "  "
                  << std::right << std::setw(10) << "Source" << "  "
                  << std::setw(10) << "Generated" << '\n';

        for (size_t i = 0; i < sections.size(); ++i) {
            const auto& s = sections[i];
            std::cout << std::setw(5) << i << ":" << "  "
                      << std::left << std::setw(static_cast<int>(w_name)) << s.Name << "  "
                      << std::right << std::setw(10) << size_str(s.Source) << "  "
                      << std::setw(10) << size_str(s.GeneratedSource) << '\n';
        }
    }
}

int main(int argc, char** argv) {
    bool show_sections = false;
    bool names_only = false;
    std::vector<std::string> entries;
    std::string path;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-s" || arg == "--sections") {
            show_sections = true;
        } else if (arg == "-n" || arg == "--names-only") {
            names_only = true;
        } else if (arg == "-e" || arg == "--entry") {
            if (i + 1 >= argc) {
                Usage(argv[0]);
                return 2;
            }
            entries.emplace_back(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            Usage(argv[0]);
            return 0;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << '\n';
            Usage(argv[0]);
            return 2;
        } else if (path.empty()) {
            path = arg;
        } else {
            Usage(argv[0]);
            return 2;
        }
    }

    if (path.empty()) {
        Usage(argv[0]);
        return 2;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open '" << path << "'\n";
        return 1;
    }

    ByteCodeReader reader(file);
    const int rc = reader.Load();
    if (rc != ByteCodeFormat::ToCode(ByteCodeFormat::ByteCodeError::None)) {
        std::cerr << path << ": " << ErrorText(rc) << " (code " << rc << ")\n";
        return 1;
    }

    const auto& symbols = reader.GetDebugSymbols();

    if (names_only) {
        for (const auto& s : symbols) {
            std::cout << SymDisplay(s) << '\n';
        }
        return 0;
    }

    std::cout << "File: " << path << "\n\n";
    if (symbols.empty()) {
        std::cout << "No debug symbols in this file.\n";
    } else {
        PrintSymbols(symbols);
    }

    if (show_sections) {
        PrintSections(reader.GetSections());
    }

    if (!entries.empty()) {
        std::cout << "\nEntry points:\n";
        for (const auto& name : entries) {
            const std::string mangled = reader.GetEntryPoint(name);
            std::cout << "  " << name << " -> "
                      << (mangled.empty() ? "<not found>" : mangled) << '\n';
        }
    }

    return 0;
}