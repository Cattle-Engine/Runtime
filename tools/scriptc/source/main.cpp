
#include "ce_common/as_compiler.hpp"

#include <angelscript.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <fstream>

namespace {
    namespace FS = std::filesystem;

    constexpr std::string_view kVersion = "0.1.0";

    struct Options {
        FS::path Input;
        FS::path Output;
        std::vector<FS::path> ImportPaths;
        bool Debug = false;
        bool Verbose = false;
        bool ShowHelp = false;
        bool ShowVersion = false;
        bool Optimize = true;
    };

    void PrintUsage(std::ostream& stream) {
        stream << "Usage: ce_scriptc [OPTIONS] [FILE]\n"
               << "       ce_scriptc [COMMAND] [OPTIONS] FILE\n\n"
               << "Commands:\n"
               << "  compile            Compile an AngelScript file\n"
               << "  build              Alias for compile\n"
               << "  help               Show this help message\n"
               << "  version            Show compiler version\n\n"
               << "Compile AngelScript source into CEBYC bytecode.\n\n"
               << "Options:\n"
               << "  -h, --help          Show this help message\n"
               << "  -V, --version       Show compiler version\n"
               << "  -o FILE             Set output file\n"
               << "  -g, --debug         Include debug information\n"
               << "  -I DIR              Add an import search directory\n"
               << "  -v, --verbose       Enable verbose output\n"
               << "  -O0                 Disable bytecode optimization\n"
               << "  -O1                 Enable bytecode optimization\n"
               << "  --                  Treat remaining arguments as positional\n";
    }

    void PrintVersion() {
        std::cout << "ce_scriptc " << kVersion << '\n'
                  << "AngelScript version: " << asGetLibraryVersion() << '\n';
    }

    bool ParseArguments(int argc, char* argv[], Options& options) {
        std::vector<std::string> positional;
        bool parse_options = true;

        for (int i = 1; i < argc; ++i) {
            const std::string_view arg(argv[i]);

            if (parse_options && arg == "--") {
                parse_options = false;
                continue;
            }

            if (parse_options && (arg == "compile" || arg == "build") && positional.empty()) {
                continue;
            }

            if (parse_options && (arg == "help" || arg == "version")) {
                if (arg == "help")
                    options.ShowHelp = true;
                else
                    options.ShowVersion = true;
                continue;
            }

            if (parse_options && (arg == "-h" || arg == "--help")) {
                options.ShowHelp = true;
                continue;
            }

            if (parse_options && (arg == "-V" || arg == "--version")) {
                options.ShowVersion = true;
                continue;
            }

            if (parse_options && (arg == "-g" || arg == "--debug")) {
                options.Debug = true;
                continue;
            }

            if (parse_options && (arg == "-v" || arg == "--verbose")) {
                options.Verbose = true;
                continue;
            }

            if (parse_options && arg == "-O0") {
                options.Optimize = false;
                continue;
            }

            if (parse_options && arg == "-O1") {
                options.Optimize = true;
                continue;
            }

            if (parse_options && arg == "-o") {
                if (++i >= argc) {
                    std::cerr << "ce_scriptc: option '-o' requires an argument\n";
                    return false;
                }

                options.Output = argv[i];
                continue;
            }

            if (parse_options && arg == "-I") {
                if (++i >= argc) {
                    std::cerr << "ce_scriptc: option '-I' requires an argument\n";
                    return false;
                }

                options.ImportPaths.emplace_back(argv[i]);
                continue;
            }

            if (parse_options && arg.starts_with("-I") && arg.size() > 2) {
                options.ImportPaths.emplace_back(arg.substr(2));
                continue;
            }

            if (parse_options && !arg.empty() && arg.front() == '-') {
                std::cerr << "ce_scriptc: unknown option '" << arg << "'\n";
                return false;
            }

            positional.emplace_back(arg);
        }

        if (options.ShowHelp || options.ShowVersion) {
            return true;
        }

        if (positional.size() != 1) {
            std::cerr << "ce_scriptc: expected exactly one input file\n";
            return false;
        }

        options.Input = positional.front();

        if (options.Output.empty()) {
            options.Output = options.Input;
            options.Output.replace_extension(".cebyc");
        }

        return true;
    }

    void MessageCallback(const asSMessageInfo* message, void*) {
        const char* section = message->section != nullptr ? message->section : "<unknown>";

        const char* type = "info";
        if (message->type == asMSGTYPE_WARNING) {
            type = "warning";
        } else if (message->type == asMSGTYPE_ERROR) {
            type = "error";
        }

        std::cerr << section << ':' << message->row << ':' << message->col
                  << ": " << type << ": " << message->message << '\n';
    }

    int Compile(const Options& options) {
        if (!FS::exists(options.Input) || !FS::is_regular_file(options.Input)) {
            std::cerr << "ce_scriptc: input file does not exist or is not a regular file: "
                      << options.Input.string() << '\n';
            return 1;
        }

        if (options.Verbose) {
            std::cout << "Input:  " << options.Input.string() << '\n'
                      << "Output: " << options.Output.string() << '\n';
        }

        std::vector<FS::path> import_paths = options.ImportPaths;

        if (import_paths.empty()) {
            import_paths.push_back(options.Input.parent_path().empty()
                                       ? FS::current_path()
                                       : options.Input.parent_path());
        }

        // Resolve imports and generate the script sections.
        CE::Scripting::Impl::ModuleImporter importer(import_paths);

        std::vector<CE::Scripting::Impl::GeneratedScriptSection> sections;

        try {
            sections = importer.LoadFile(options.Input.filename().string());
        } catch (const std::exception& exception) {
            std::cerr << "ce_scriptc: failed to load scripts: "
                      << exception.what() << '\n';
            return 1;
        }

        if (sections.empty()) {
            std::cerr << "ce_scriptc: no script sections were generated\n";
            return 1;
        }

        if (options.Verbose) {
            std::cout << "Generated " << sections.size() << " script section(s)\n";
        }

        // Create the AngelScript engine.
        asIScriptEngine* engine = asCreateScriptEngine();

        if (engine == nullptr) {
            std::cerr << "ce_scriptc: failed to create AngelScript engine\n";
            return 1;
        }

        engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);
        engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, options.Optimize ? 1 : 0);
        engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false);

        asIScriptModule* module = engine->GetModule("ce_scriptc", asGM_ALWAYS_CREATE);

        if (module == nullptr) {
            std::cerr << "ce_scriptc: failed to create script module\n";
            engine->ShutDownAndRelease();
            return 1;
        }

        for (const auto& section : sections) {
            const int result = module->AddScriptSection(
                section.Name.c_str(),
                section.Code.data(),
                section.Code.size()
            );

            if (result < 0) {
                std::cerr << "ce_scriptc: failed to add script section '"
                          << section.Name << "'\n";
                engine->ShutDownAndRelease();
                return 1;
            }
        }

        if (options.Verbose) {
            std::cout << "Building AngelScript module...\n";
        }

        const int build_result = module->Build();

        if (build_result < 0) {
            std::cerr << "ce_scriptc: compilation failed\n";
            engine->ShutDownAndRelease();
            return 1;
        }

        // Create the output directory if necessary.
        std::error_code filesystem_error;
        if (!options.Output.parent_path().empty()) {
            FS::create_directories(options.Output.parent_path(), filesystem_error);

            if (filesystem_error) {
                std::cerr << "ce_scriptc: failed to create output directory: "
                          << filesystem_error.message() << '\n';
                engine->ShutDownAndRelease();
                return 1;
            }
        }

        // debug-symbol table.
        {
            std::ofstream output(options.Output, std::ios::binary);

            if (!output) {
                std::cerr << "ce_scriptc: cannot open output file: "
                          << options.Output.string() << '\n';
                engine->ShutDownAndRelease();
                return 1;
            }

            CE::Scripting::Impl::ByteCodeWriter writer(output);

            const int save_result = module->SaveByteCode(
                &writer.GetByteCodeSink(),
                !options.Debug
            );

            if (save_result < 0) {
                std::cerr << "ce_scriptc: failed to serialize AngelScript bytecode\n";
                engine->ShutDownAndRelease();
                return 1;
            }

            if (options.Debug) {
                writer.SetDebugSymbols(importer.GetGeneratedSymbols());
            }

            const std::string entry_point = importer.GetGeneratedEntrypoint("main");

            if (!entry_point.empty()) {
                writer.AddEntryPoint("main", entry_point);
            }

            if (!writer.Write()) {
                std::cerr << "ce_scriptc: failed to write CEBYC output\n";
                engine->ShutDownAndRelease();
                return 1;
            }
        }

        engine->ShutDownAndRelease();

        if (options.Verbose) {
            std::cout << "Compilation successful\n"
                      << "Wrote: " << options.Output.string() << '\n';
        }

        return 0;
    }
}

int main(int argc, char* argv[]) {
    Options options;

    if (!ParseArguments(argc, argv, options)) {
        PrintUsage(std::cerr);
        return 2;
    }

    if (options.ShowHelp) {
        PrintUsage(std::cout);
        return 0;
    }

    if (options.ShowVersion) {
        PrintVersion();
        return 0;
    }

    return Compile(options);
}