#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <SDL3/SDL_stdinc.h>
#include <SDL3_shadercross/SDL_shadercross.h>

namespace fs = std::filesystem;

struct Options {
    fs::path input;
    fs::path output_dir = "compiled_shaders";
    std::string stage;
    std::string entry = "main";
    std::string target_env = "vulkan1.0";
    std::vector<std::string> formats = {"SPIRV", "MSL", "DXIL"};
    std::vector<std::string> include_dirs;
    bool debug = false;
    bool help = false;
};

static std::string Lower(std::string v) {
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return v;
}
static std::string Upper(std::string v) {
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return (char)std::toupper(c); });
    return v;
}

static void PrintHelp() {
    std::cout
        << "ce_shaderc - GLSL to SPIR-V / MSL / HLSL / DXIL / DXBC (self-contained)\n\n"
        << "Usage:\n"
        << "  ce_shaderc <input.glsl> [options]\n\n"
        << "Options:\n"
        << "  -o, --output-dir <dir>     Output directory (default: compiled_shaders)\n"
        << "  -t, --stage <stage>        vertex, fragment, compute\n"
        << "  -e, --entry <name>         Entry point (default: main)\n"
        << "  -I <dir>                   Add #include search directory (repeatable)\n"
        << "  --formats <list>           Comma-separated: spv,msl,dxil,dxbc,hlsl,json\n"
        << "                             Default: spv,msl,dxil\n"
        << "  --all                      Generate every format\n"
        << "  --target-env <env>         vulkan1.0|1.1|1.2|1.3 (default: vulkan1.0)\n"
        << "  -g, --debug                Keep debug info\n"
        << "  -h, --help                 Show this help\n";
}

static bool TakeValue(int& i, int argc, char** argv, std::string& out) {
    if (i + 1 >= argc) {
        std::cerr << "ce_shaderc: missing value for " << argv[i] << '\n';
        return false;
    }
    out = argv[++i];
    return true;
}

static bool ParseFormats(const std::string& text, std::vector<std::string>& formats) {
    formats.clear();
    std::string item;
    auto flush = [&] {
        if (!item.empty()) formats.push_back(Upper(item));
        item.clear();
    };
    for (char c : text) {
        if (c == ',') flush();
        else if (!std::isspace((unsigned char)c)) item += c;
    }
    flush();

    static const std::set<std::string> valid = {"SPV", "SPIRV", "MSL", "DXIL", "DXBC", "HLSL", "JSON"};
    for (auto& f : formats) {
        if (!valid.contains(f)) {
            std::cerr << "ce_shaderc: unknown output format: " << f << '\n';
            return false;
        }
        if (f == "SPV") f = "SPIRV";
    }
    std::vector<std::string> unique;
    for (auto& f : formats)
        if (std::find(unique.begin(), unique.end(), f) == unique.end()) unique.push_back(f);
    formats = std::move(unique);
    return !formats.empty();
}

static bool ParseArgs(int argc, char** argv, Options& o) {
    if (argc < 2) { o.help = true; return true; }
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        std::string value;
        if (arg == "-h" || arg == "--help") { o.help = true; return true; }
        else if (arg == "-o" || arg == "--output-dir") { if (!TakeValue(i, argc, argv, value)) return false; o.output_dir = value; }
        else if (arg == "-t" || arg == "--stage") { if (!TakeValue(i, argc, argv, o.stage)) return false; o.stage = Lower(o.stage); }
        else if (arg == "-e" || arg == "--entry") { if (!TakeValue(i, argc, argv, o.entry)) return false; }
        else if (arg == "--formats") { if (!TakeValue(i, argc, argv, value)) return false; if (!ParseFormats(value, o.formats)) return false; }
        else if (arg == "--all") { o.formats = {"SPIRV", "MSL", "DXIL", "DXBC", "HLSL", "JSON"}; }
        else if (arg == "--target-env") { if (!TakeValue(i, argc, argv, o.target_env)) return false; o.target_env = Lower(o.target_env); }
        else if (arg == "-g" || arg == "--debug") { o.debug = true; }
        else if (arg == "-I") { if (!TakeValue(i, argc, argv, value)) return false; o.include_dirs.push_back(value); }
        else if (arg.rfind("-I", 0) == 0 && arg.size() > 2) { o.include_dirs.push_back(arg.substr(2)); }
        else if (!arg.empty() && arg[0] == '-') { std::cerr << "ce_shaderc: unknown option: " << arg << '\n'; return false; }
        else if (o.input.empty()) { o.input = arg; }
        else { std::cerr << "ce_shaderc: unexpected extra input: " << arg << '\n'; return false; }
    }
    if (o.input.empty()) { std::cerr << "ce_shaderc: no input file specified\n"; return false; }
    return true;
}

static std::string InferStage(const fs::path& p) {
    const std::string e = Lower(p.extension().string());
    if (e == ".vert") return "vertex";
    if (e == ".frag") return "fragment";
    if (e == ".comp") return "compute";
    return {};
}
static std::string StageSuffix(const std::string& s) {
    if (s == "vertex") return "vert";
    if (s == "fragment") return "frag";
    if (s == "compute") return "comp";
    return {};
}
static std::string OutputExtension(const std::string& f) { return f == "SPIRV" ? "spv" : Lower(f); }

static bool ReadFile(const fs::path& p, std::string& out) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}
static bool WriteFile(const fs::path& p, const void* data, size_t size) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(static_cast<const char*>(data), (std::streamsize)size);
    return (bool)f;
}

class FileIncluder : public glslang::TShader::Includer {
  public:
    FileIncluder(std::vector<fs::path> dirs) : mDirs(std::move(dirs)) {}

    IncludeResult* includeLocal(const char* header, const char* includer, size_t depth) override {
        (void)depth;
        std::vector<fs::path> candidates;
        if (includer && *includer) candidates.push_back(fs::path(includer).parent_path() / header);
        for (auto& d : mDirs) candidates.push_back(d / header);
        return TryOpen(header, candidates);
    }
    IncludeResult* includeSystem(const char* header, const char*, size_t) override {
        std::vector<fs::path> candidates;
        for (auto& d : mDirs) candidates.push_back(d / header);
        return TryOpen(header, candidates);
    }
    void releaseInclude(IncludeResult* r) override {
        if (!r) return;
        delete static_cast<std::string*>(r->userData);
        delete r;
    }

  private:
    IncludeResult* TryOpen(const char* header, const std::vector<fs::path>& candidates) {
        for (auto& c : candidates) {
            std::error_code ec;
            if (!fs::is_regular_file(c, ec)) continue;
            auto* content = new std::string();
            if (!ReadFile(c, *content)) { delete content; continue; }
            // Store full path as the header name so nested relative includes resolve.
            auto* name = content; // keep content alive through userData
            return new IncludeResult(fs::weakly_canonical(c, ec).string(), name->data(), name->size(), name);
        }
        (void)header;
        return nullptr;
    }
    std::vector<fs::path> mDirs;
};


static bool CompileGlsl(const Options& o, const std::string& source, std::vector<uint32_t>& spirv) {
    EShLanguage lang;
    if (o.stage == "vertex") lang = EShLangVertex;
    else if (o.stage == "fragment") lang = EShLangFragment;
    else lang = EShLangCompute;

    glslang::EShTargetClientVersion client_ver = glslang::EShTargetVulkan_1_0;
    glslang::EShTargetLanguageVersion spv_ver = glslang::EShTargetSpv_1_0;
    if (o.target_env == "vulkan1.0") { client_ver = glslang::EShTargetVulkan_1_0; spv_ver = glslang::EShTargetSpv_1_0; }
    else if (o.target_env == "vulkan1.1") { client_ver = glslang::EShTargetVulkan_1_1; spv_ver = glslang::EShTargetSpv_1_3; }
    else if (o.target_env == "vulkan1.2") { client_ver = glslang::EShTargetVulkan_1_2; spv_ver = glslang::EShTargetSpv_1_5; }
    else if (o.target_env == "vulkan1.3") { client_ver = glslang::EShTargetVulkan_1_3; spv_ver = glslang::EShTargetSpv_1_6; }
    else {
        std::cerr << "ce_shaderc: unsupported --target-env: " << o.target_env << '\n';
        return false;
    }

    const std::string name = o.input.string();
    const char* text = source.c_str();
    const int length = (int)source.size();
    const char* nameC = name.c_str();

    glslang::TShader shader(lang);
    shader.setStringsWithLengthsAndNames(&text, &length, &nameC, 1);
    shader.setEntryPoint(o.entry.c_str());
    shader.setSourceEntryPoint(o.entry.c_str());
    shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, client_ver);
    shader.setEnvTarget(glslang::EShTargetSpv, spv_ver);

    std::vector<fs::path> dirs;
    for (auto& d : o.include_dirs) dirs.emplace_back(d);
    dirs.push_back(o.input.parent_path());
    FileIncluder includer(dirs);

    const TBuiltInResource* resources = GetDefaultResources();
    const EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    if (!shader.parse(resources, 100, false, messages, includer)) {
        std::cerr << shader.getInfoLog() << shader.getInfoDebugLog();
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        std::cerr << program.getInfoLog() << program.getInfoDebugLog();
        return false;
    }

    glslang::TIntermediate* inter = program.getIntermediate(lang);
    if (!inter) {
        std::cerr << "ce_shaderc: no intermediate for stage\n";
        return false;
    }

    glslang::SpvOptions spv_opts;
    spv_opts.generateDebugInfo = o.debug;
    spv_opts.stripDebugInfo = !o.debug;
    spv_opts.disableOptimizer = true;
    spv_opts.optimizeSize = false;
    spv_opts.validate = true;

    spv::SpvBuildLogger logger;
    glslang::GlslangToSpv(*inter, spirv, &logger, &spv_opts);
    const std::string msgs = logger.getAllMessages();
    if (!msgs.empty()) std::cerr << msgs << '\n';
    return !spirv.empty();
}

static std::string BuildReflectionJson(const Options& o, const std::vector<uint32_t>& spirv) {
    const auto* bytes = reinterpret_cast<const Uint8*>(spirv.data());
    const size_t size = spirv.size() * sizeof(uint32_t);
    std::ostringstream js;

    if (o.stage == "compute") {
        auto* m = SDL_ShaderCross_ReflectComputeSPIRV(bytes, size, 0);
        if (!m) return {};
        js << "{\n"
           << "  \"stage\": \"compute\",\n"
           << "  \"samplers\": " << m->num_samplers << ",\n"
           << "  \"readonly_storage_textures\": " << m->num_readonly_storage_textures << ",\n"
           << "  \"readonly_storage_buffers\": " << m->num_readonly_storage_buffers << ",\n"
           << "  \"readwrite_storage_textures\": " << m->num_readwrite_storage_textures << ",\n"
           << "  \"readwrite_storage_buffers\": " << m->num_readwrite_storage_buffers << ",\n"
           << "  \"uniform_buffers\": " << m->num_uniform_buffers << ",\n"
           << "  \"threadcount\": [" << m->threadcount_x << ", " << m->threadcount_y << ", " << m->threadcount_z << "]\n"
           << "}\n";
        SDL_free(m);
    } else {
        auto* m = SDL_ShaderCross_ReflectGraphicsSPIRV(bytes, size, 0);
        if (!m) return {};
        js << "{\n"
           << "  \"stage\": \"" << o.stage << "\",\n"
           << "  \"samplers\": " << m->resource_info.num_samplers << ",\n"
           << "  \"storage_textures\": " << m->resource_info.num_storage_textures << ",\n"
           << "  \"storage_buffers\": " << m->resource_info.num_storage_buffers << ",\n"
           << "  \"uniform_buffers\": " << m->resource_info.num_uniform_buffers << "\n"
           << "}\n";
        SDL_free(m);
    }
    return js.str();
}

int main(int argc, char** argv) {
    Options o;
    if (!ParseArgs(argc, argv, o)) { PrintHelp(); return 2; }
    if (o.help) { PrintHelp(); return 0; }

    if (!fs::exists(o.input) || !fs::is_regular_file(o.input)) {
        std::cerr << "ce_shaderc: input file does not exist: " << o.input << '\n';
        return 1;
    }
    if (o.stage.empty()) o.stage = InferStage(o.input);
    const std::string suffix = StageSuffix(o.stage);
    if (suffix.empty()) {
        std::cerr << "ce_shaderc: cannot infer stage. Use .vert/.frag/.comp or --stage.\n";
        return 2;
    }

    std::error_code ec;
    fs::create_directories(o.output_dir, ec);
    if (ec) {
        std::cerr << "ce_shaderc: cannot create output directory: " << ec.message() << '\n';
        return 1;
    }

    std::string source;
    if (!ReadFile(o.input, source)) {
        std::cerr << "ce_shaderc: cannot read " << o.input << '\n';
        return 1;
    }

    const std::string stem = o.input.stem().string();
    const bool needs_cross = std::any_of(o.formats.begin(), o.formats.end(),
        [](const std::string& f) { return f != "SPIRV"; });

    // --- GLSL -> SPIR-V ---
    std::cout << "[GLSL -> SPIR-V] " << o.input << '\n';
    glslang::InitializeProcess();
    std::vector<uint32_t> spirv;
    const bool ok = CompileGlsl(o, source, spirv);
    glslang::FinalizeProcess();
    if (!ok) {
        std::cerr << "ce_shaderc: GLSL compilation failed.\n";
        return 1;
    }

    const size_t spv_bytes = spirv.size() * sizeof(uint32_t);
    const fs::path spirv_path = o.output_dir / (stem + "." + suffix + ".spv");
    if (std::find(o.formats.begin(), o.formats.end(), "SPIRV") != o.formats.end()) {
        if (!WriteFile(spirv_path, spirv.data(), spv_bytes)) {
            std::cerr << "ce_shaderc: failed to write " << spirv_path << '\n';
            return 1;
        }
        std::cout << "[OK] " << spirv_path << '\n';
    }

    if (!needs_cross) {
        std::cout << "ce_shaderc: compilation complete.\n";
        return 0;
    }

    if (!SDL_ShaderCross_Init()) {
        std::cerr << "ce_shaderc: SDL_ShaderCross_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_ShaderCross_ShaderStage cross_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    if (o.stage == "fragment") cross_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
    else if (o.stage == "compute") cross_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;

    SDL_ShaderCross_SPIRV_Info info{};
    info.bytecode = reinterpret_cast<const Uint8*>(spirv.data());
    info.bytecode_size = spv_bytes;
    info.entrypoint = o.entry.c_str();
    info.shader_stage = cross_stage;
    info.props = 0;

    int rc = 0;
    for (const auto& format : o.formats) {
        if (format == "SPIRV") continue;
        const fs::path out = o.output_dir / (stem + "." + suffix + "." + OutputExtension(format));
        std::cout << "[SPIR-V -> " << format << "]\n";

        void* data = nullptr;
        size_t size = 0;
        std::string json;

        if (format == "MSL") {
            data = SDL_ShaderCross_TranspileMSLFromSPIRV(&info);
            if (data) size = std::strlen(static_cast<const char*>(data));
        } else if (format == "HLSL") {
            data = SDL_ShaderCross_TranspileHLSLFromSPIRV(&info);
            if (data) size = std::strlen(static_cast<const char*>(data));
        } else if (format == "DXBC") {
            data = SDL_ShaderCross_CompileDXBCFromSPIRV(&info, &size);
        } else if (format == "DXIL") {
            data = SDL_ShaderCross_CompileDXILFromSPIRV(&info, &size);
        } else if (format == "JSON") {
            json = BuildReflectionJson(o, spirv);
            if (!json.empty()) { data = json.data(); size = json.size(); }
        }

        if (!data || size == 0) {
            std::cerr << "ce_shaderc: failed producing " << format << ": " << SDL_GetError() << '\n';
            rc = 1;
            if (format != "JSON") SDL_free(data);
            break;
        }
        const bool wrote = WriteFile(out, data, size);
        if (format != "JSON") SDL_free(data);
        if (!wrote) {
            std::cerr << "ce_shaderc: failed to write " << out << '\n';
            rc = 1;
            break;
        }
        std::cout << "[OK] " << out << '\n';
    }

    SDL_ShaderCross_Quit();
    if (rc == 0) std::cout << "ce_shaderc: compilation complete.\n";
    return rc;
}