#include "common/ini.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>

namespace CE::Ini {
namespace {

static std::string_view trim(std::string_view s)
{
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
        b++;

    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
        e--;

    return s.substr(b, e - b);
}

static bool starts_with(std::string_view s, char c)
{
    return !s.empty() && s.front() == c;
}

static std::string unescape(std::string_view s)
{
    std::string out;
    out.reserve(s.size());

    for (size_t i = 0; i < s.size(); i++) {
        char ch = s[i];
        if (ch != '\\' || i + 1 >= s.size()) {
            out.push_back(ch);
            continue;
        }

        char n = s[++i];
        switch (n) {
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        case '\\': out.push_back('\\'); break;
        case '"': out.push_back('"'); break;
        case '\'': out.push_back('\''); break;
        default:
            out.push_back(n);
            break;
        }
    }

    return out;
}

static std::string normalize_value(std::string_view v, Options opt)
{
    (void)opt;
    v = trim(v);
    if (v.size() >= 2 && ((v.front() == '"' && v.back() == '"') || (v.front() == '\'' && v.back() == '\''))) {
        v = v.substr(1, v.size() - 2);
        return unescape(v);
    }
    return std::string(v);
}

static std::string_view strip_inline_comment(std::string_view v)
{
    bool in_quotes = false;
    char q = 0;

    for (size_t i = 0; i < v.size(); i++) {
        char ch = v[i];

        if (!in_quotes && (ch == '"' || ch == '\'')) {
            in_quotes = true;
            q = ch;
            continue;
        }

        if (in_quotes) {
            if (ch == q) {
                size_t bs = 0;
                for (size_t k = i; k > 0 && v[k - 1] == '\\'; k--)
                    bs++;
                if ((bs % 2) == 0) {
                    in_quotes = false;
                    q = 0;
                }
            }
            continue;
        }

        if (ch == ';' || ch == '#') {
            if (i == 0 || std::isspace(static_cast<unsigned char>(v[i - 1])))
                return v.substr(0, i);
        }
    }

    return v;
}

static bool parse_impl(std::string_view text, IniFile& out, ParseError* err, Options opt)
{
    out.clear();

    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.remove_prefix(3);
    }

    std::string current_section;
    size_t line_no = 1;
    size_t pos = 0;

    while (pos <= text.size()) {
        size_t line_end = text.find('\n', pos);
        if (line_end == std::string_view::npos)
            line_end = text.size();

        std::string_view line = text.substr(pos, line_end - pos);
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);

        pos = (line_end < text.size()) ? (line_end + 1) : (text.size() + 1);

        std::string_view t = trim(line);
        if (t.empty()) {
            line_no++;
            continue;
        }

        if (starts_with(t, ';') || starts_with(t, '#')) {
            line_no++;
            continue;
        }

        if (starts_with(t, '[')) {
            size_t close = t.find(']');
            if (close == std::string_view::npos) {
                if (err) {
                    err->line = line_no;
                    err->column = 1;
                    err->message = "Missing closing ']'";
                }
                return false;
            }

            std::string_view sec = trim(t.substr(1, close - 1));
            current_section = std::string(sec);
            line_no++;
            continue;
        }

        size_t eq = t.find('=');
        size_t colon = opt.allow_colon_delim ? t.find(':') : std::string_view::npos;

        size_t delim = std::string_view::npos;
        if (eq != std::string_view::npos && colon != std::string_view::npos)
            delim = std::min(eq, colon);
        else if (eq != std::string_view::npos)
            delim = eq;
        else if (colon != std::string_view::npos)
            delim = colon;

        if (delim == std::string_view::npos) {
            if (err) {
                err->line = line_no;
                err->column = 1;
                err->message = "Expected key/value delimiter '=' (or ':' if enabled)";
            }
            return false;
        }

        std::string_view key = trim(t.substr(0, delim));
        std::string_view value = t.substr(delim + 1);

        if (key.empty()) {
            if (err) {
                err->line = line_no;
                err->column = 1;
                err->message = "Empty key";
            }
            return false;
        }

        if (opt.allow_inline_comments)
            value = strip_inline_comment(value);
        value = trim(value);

        if (!opt.allow_empty_values && value.empty()) {
            if (err) {
                err->line = line_no;
                err->column = delim + 2;
                err->message = "Empty value not allowed";
            }
            return false;
        }

        std::string k(key);
        std::string v = normalize_value(value, opt);

        if (current_section.empty()) {
            out.global[std::move(k)] = std::move(v);
        } else {
            out.sections[current_section][std::move(k)] = std::move(v);
        }

        line_no++;
    }

    return true;
}

static bool parse_int64(std::string_view s, int64_t& out)
{
    s = trim(s);
    if (s.empty())
        return false;

    int64_t value = 0;
    auto first = s.data();
    auto last = s.data() + s.size();
    auto res = std::from_chars(first, last, value, 10);
    if (res.ec != std::errc() || res.ptr != last)
        return false;

    out = value;
    return true;
}

static bool parse_double(std::string_view s, double& out)
{
    s = trim(s);
    if (s.empty())
        return false;

    std::string tmp(s);
    char* end = nullptr;
    errno = 0;
    double v = std::strtod(tmp.c_str(), &end);
    if (errno != 0 || !end || *end != '\0')
        return false;

    out = v;
    return true;
}

static bool parse_bool(std::string_view s, bool& out)
{
    s = trim(s);
    std::string tmp(s);
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (tmp == "1" || tmp == "true" || tmp == "yes" || tmp == "on") {
        out = true;
        return true;
    }
    if (tmp == "0" || tmp == "false" || tmp == "no" || tmp == "off") {
        out = false;
        return true;
    }

    return false;
}

} 

void IniFile::clear()
{
    global.clear();
    sections.clear();
}

bool IniFile::has(std::string_view section, std::string_view key) const
{
    return get_ptr(section, key) != nullptr;
}

const std::string* IniFile::get_ptr(std::string_view section, std::string_view key) const
{
    if (key.empty())
        return nullptr;

    if (section.empty()) {
        auto it = global.find(std::string(key));
        return (it == global.end()) ? nullptr : &it->second;
    }

    auto sit = sections.find(std::string(section));
    if (sit == sections.end())
        return nullptr;

    auto kit = sit->second.find(std::string(key));
    return (kit == sit->second.end()) ? nullptr : &kit->second;
}

std::string IniFile::get_string(std::string_view section,
                                std::string_view key,
                                std::string_view def) const
{
    const std::string* p = get_ptr(section, key);
    if (!p)
        return std::string(def);
    return *p;
}

int64_t IniFile::get_int(std::string_view section, std::string_view key, int64_t def) const
{
    const std::string* p = get_ptr(section, key);
    if (!p)
        return def;

    int64_t v = 0;
    if (!parse_int64(*p, v))
        return def;
    return v;
}

double IniFile::get_float(std::string_view section, std::string_view key, double def) const
{
    const std::string* p = get_ptr(section, key);
    if (!p)
        return def;

    double v = 0.0;
    if (!parse_double(*p, v))
        return def;
    return v;
}

bool IniFile::get_bool(std::string_view section, std::string_view key, bool def) const
{
    const std::string* p = get_ptr(section, key);
    if (!p)
        return def;

    bool v = false;
    if (!parse_bool(*p, v))
        return def;
    return v;
}

bool parse_memory(const void* data,
                  size_t size,
                  IniFile& out,
                  ParseError* err,
                  Options opt)
{
    if (!data && size != 0) {
        if (err) {
            err->line = 0;
            err->column = 0;
            err->message = "Null data with non-zero size";
        }
        return false;
    }

    std::string_view sv(static_cast<const char*>(data), size);
    return parse_impl(sv, out, err, opt);
}

bool parse(std::string_view text, IniFile& out, ParseError* err, Options opt)
{
    return parse_impl(text, out, err, opt);
}

bool load_file(const std::filesystem::path& path, IniFile& out, ParseError* err, Options opt)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        if (err) {
            err->line = 0;
            err->column = 0;
            err->message = "Failed to open file";
        }
        return false;
    }

    f.seekg(0, std::ios::end);
    std::streamoff sz = f.tellg();
    if (sz < 0) {
        if (err) {
            err->line = 0;
            err->column = 0;
            err->message = "Failed to stat file size";
        }
        return false;
    }
    f.seekg(0, std::ios::beg);

    std::string buf;
    buf.resize(static_cast<size_t>(sz));
    if (!buf.empty()) {
        f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        if (!f || f.gcount() != static_cast<std::streamsize>(buf.size())) {
            if (err) {
                err->line = 0;
                err->column = 0;
                err->message = "Failed to read file";
            }
            return false;
        }
    }

    return parse_impl(std::string_view(buf), out, err, opt);
}

}
