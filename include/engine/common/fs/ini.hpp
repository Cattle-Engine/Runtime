#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace CE::Ini {

struct ParseError {
    size_t line = 0;   
    size_t column = 0;
    std::string message;
};

struct Options {
    bool allow_colon_delim = true;    
    bool allow_inline_comments = true; 
    bool allow_empty_values = true;    
};

struct WriteOptions {
    bool sort = true;
    bool space_around_delim = true;
    char delim = '=';
    char comment_prefix = ';';
    bool newline_at_eof = true;
};

using Section = std::unordered_map<std::string, std::string>;

class IniFile {
public:
    Section global;
    std::unordered_map<std::string, Section> sections;

    std::vector<std::string> header_comments;
    std::unordered_map<std::string, std::string> section_comments;
    std::unordered_map<std::string, Section> key_comments;

    void clear();

    bool has(std::string_view section, std::string_view key) const;
    const std::string* get_ptr(std::string_view section, std::string_view key) const;

    std::string get_string(std::string_view section,
                           std::string_view key,
                           std::string_view def = {}) const;
    int64_t get_int(std::string_view section, std::string_view key, int64_t def = 0) const;
    double get_float(std::string_view section, std::string_view key, double def = 0.0) const;
    bool get_bool(std::string_view section, std::string_view key, bool def = false) const;

    void set_string(std::string_view section, std::string_view key, std::string_view value);
    void set_int(std::string_view section, std::string_view key, int64_t value);
    void set_float(std::string_view section, std::string_view key, double value);
    void set_bool(std::string_view section, std::string_view key, bool value);
    void erase(std::string_view section, std::string_view key);

    void add_header_comment(std::string_view comment);
    void set_section_comment(std::string_view section, std::string_view comment);
    void set_key_comment(std::string_view section, std::string_view key, std::string_view comment);
};

bool parse_memory(const void* data,
                  size_t size,
                  IniFile& out,
                  ParseError* err = nullptr,
                  Options opt = {});

bool parse(std::string_view text, IniFile& out, ParseError* err = nullptr, Options opt = {});

bool load_file(const std::filesystem::path& path, IniFile& out, ParseError* err = nullptr, Options opt = {});

std::string serialize(const IniFile& ini, WriteOptions opt = {});
bool save_file(const std::filesystem::path& path, const IniFile& ini, WriteOptions opt = {});

}
