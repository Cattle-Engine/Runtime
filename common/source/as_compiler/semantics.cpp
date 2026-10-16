#include "ce_common/as_compiler/semantics.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <limits>
#include <optional>
#include <unordered_set>

#include "ce_common/detail/tracelog.hpp"
#include "ce_common/detail/hasher.hpp"
#include "ce_common/as_compiler/parser.hpp"
#include "ce_common/as_compiler/ast.hpp"
#include "ce_common/as_compiler/common.hpp"
#include "ce_common/as_compiler/exceptions.hpp"
#include "ce_common/as_compiler/lexer.hpp"

namespace CE::Scripting::Impl::Semantics {
    namespace {
        constexpr char kInternalFunctionPrefix[] = "__ce_mod_f_";
        constexpr char kInternalGlobalPrefix[] = "__ce_mod_g_";
        constexpr char kInternalTypePrefix[] = "__ce_mod_t_";

        std::string StripNamespace(const std::string& name) {
            size_t pos = name.rfind("::");
            if (pos != std::string::npos) {
                return name.substr(pos + 2);
            }
            return name;
        }

        const AST::ASTDeclaration* FindDeclarationInDecls(const std::vector<AST::ASTDeclaration>& decls,
                                                          const std::string& target_qualified_name,
                                                          const std::string& current_namespace) {
            for (const auto& decl : decls) {
                std::string full_name = current_namespace.empty() ? decl.Name : current_namespace + "::" + decl.Name;
                if (decl.Type == AST::ASTDeclaration::Kind::Namespace) {
                    auto ns = std::get<std::shared_ptr<AST::ASTNamespace>>(decl.Data);
                    if (const auto* found =
                            FindDeclarationInDecls(ns->Declarations, target_qualified_name, full_name)) {
                        return found;
                    }
                } else {
                    if (full_name == target_qualified_name) {
                        return &decl;
                    }
                }
            }
            return nullptr;
        }

        const AST::ASTDeclaration* FindDeclarationInModule(const AST::ASTModule& module,
                                                           const std::string& qualified_name) {
            return FindDeclarationInDecls(module.Declarations, qualified_name, "");
        }

        // ====================================================================
        // Compile-time expression evaluation (used by static_assert)
        // ====================================================================

        // A typed compile-time value. Int is 64-bit signed with checked overflow.
        struct ConstValue {
            enum class Kind { Int, Float, Bool, String };

            Kind K = Kind::Int;
            int64_t I = 0;
            double F = 0.0;
            bool B = false;
            std::string S;

            static ConstValue MakeInt(int64_t v) {
                ConstValue c;
                c.K = Kind::Int;
                c.I = v;
                return c;
            }
            static ConstValue MakeFloat(double v) {
                ConstValue c;
                c.K = Kind::Float;
                c.F = v;
                return c;
            }
            static ConstValue MakeBool(bool v) {
                ConstValue c;
                c.K = Kind::Bool;
                c.B = v;
                return c;
            }
            static ConstValue MakeString(std::string v) {
                ConstValue c;
                c.K = Kind::String;
                c.S = std::move(v);
                return c;
            }

            bool IsNumeric() const { return K == Kind::Int || K == Kind::Float; }
            double AsDouble() const { return K == Kind::Int ? static_cast<double>(I) : F; }
        };

        const char* KindName(ConstValue::Kind k) {
            switch (k) {
            case ConstValue::Kind::Int:    return "int";
            case ConstValue::Kind::Float:  return "float";
            case ConstValue::Kind::Bool:   return "bool";
            case ConstValue::Kind::String: return "string";
            }
            return "unknown";
        }

        struct EvalTok {
            enum class Kind { Number, Ident, String, Op, LParen, RParen } Type;
            std::string Text;
        };

        using ConstResolver = std::function<std::optional<ConstValue>(const std::string&)>;

        // Lexers often emit operators as single chars (e.g. '&' '&'), so merge runs of
        // operator-ish tokens and re-split them greedily against the known operator set.
        // Also joins `A :: B` into a single identifier `A::B`.
        std::vector<EvalTok> NormaliseTokens(const std::vector<Lexer::Token>& in) {
            using TT = Lexer::Token::TokenType;
            static const char* kOps[] = {"&&", "||", "==", "!=", "<=", ">=", "<", ">", "+",
                                         "-",  "*",  "/",  "%",  "!",  "="};
            std::vector<EvalTok> out;
            std::string run;
            bool join_next = false; // previous token was '::'

            auto flush = [&]() {
                size_t i = 0;
                while (i < run.size()) {
                    bool matched = false;
                    for (const char* op : kOps) {
                        size_t len = std::char_traits<char>::length(op);
                        if (run.compare(i, len, op) == 0) {
                            out.push_back({EvalTok::Kind::Op, op});
                            i += len;
                            matched = true;
                            break;
                        }
                    }
                    if (!matched) {
                        // unknown operator char; emit as-is so the parser reports it
                        out.push_back({EvalTok::Kind::Op, std::string(1, run[i])});
                        ++i;
                    }
                }
                run.clear();
            };

            for (const auto& t : in) {
                if (t.Type == TT::Symbol || t.Type == TT::Assignment || t.Type == TT::Reference) {
                    run += (t.Type == TT::Assignment ? "=" : t.Type == TT::Reference ? "&" : t.Value);
                    join_next = false;
                    continue;
                }
                flush();

                switch (t.Type) {
                case TT::ScopeResolution:
                    if (!out.empty() && out.back().Type == EvalTok::Kind::Ident) {
                        out.back().Text += "::";
                        join_next = true;
                    } else {
                        out.push_back({EvalTok::Kind::Op, "::"}); // parser will reject
                    }
                    continue;
                case TT::Identifier:
                    if (join_next && !out.empty() && out.back().Type == EvalTok::Kind::Ident)
                        out.back().Text += t.Value;
                    else
                        out.push_back({EvalTok::Kind::Ident, t.Value});
                    break;
                case TT::Number:     out.push_back({EvalTok::Kind::Number, t.Value}); break;
                case TT::String:     out.push_back({EvalTok::Kind::String, t.Value}); break;
                case TT::OpenParen:  out.push_back({EvalTok::Kind::LParen, "("}); break;
                case TT::CloseParen: out.push_back({EvalTok::Kind::RParen, ")"}); break;
                case TT::Semicolon:  break; // ignore stray terminators
                default:
                    out.push_back({EvalTok::Kind::Op, t.Value}); // will be rejected by the parser
                    break;
                }
                join_next = false;
            }
            flush();
            return out;
        }

        class ConstExprEvaluator {
        public:
            ConstExprEvaluator(std::vector<EvalTok> toks, ConstResolver resolver, SourceLocation loc)
                : mToks(std::move(toks)), mResolver(std::move(resolver)), mLoc(loc) {}

            ConstValue Evaluate() {
                if (mToks.empty())
                    Fail("static_assert has an empty condition");
                ConstValue v = ParseOr();
                if (mPos != mToks.size())
                    Fail("Unexpected '" + mToks[mPos].Text + "' in static_assert condition");
                return v;
            }

        private:
            std::vector<EvalTok> mToks;
            ConstResolver mResolver;
            SourceLocation mLoc;
            size_t mPos = 0;

            [[noreturn]] void Fail(const std::string& msg) const { throw Exceptions::SemanticError(msg, mLoc); }

            bool PeekOp(const char* op) const {
                return mPos < mToks.size() && mToks[mPos].Type == EvalTok::Kind::Op && mToks[mPos].Text == op;
            }
            bool PeekWord(const char* word) const {
                return mPos < mToks.size() && mToks[mPos].Type == EvalTok::Kind::Ident && mToks[mPos].Text == word;
            }
            bool MatchOp(const char* op) {
                if (PeekOp(op)) {
                    ++mPos;
                    return true;
                }
                return false;
            }
            bool MatchOpOrWord(const char* op, const char* word) {
                if (PeekOp(op) || PeekWord(word)) {
                    ++mPos;
                    return true;
                }
                return false;
            }

            // ---------------------------------------------------------------
            // Typed helpers
            // ---------------------------------------------------------------

            bool RequireBool(const ConstValue& v, const char* op) const {
                if (v.K != ConstValue::Kind::Bool)
                    Fail(std::string("Operator '") + op + "' requires bool operands, got " + KindName(v.K));
                return v.B;
            }

            ConstValue Arith(char op, const ConstValue& a, const ConstValue& b) const {
                using K = ConstValue::Kind;

                // string concatenation
                if (a.K == K::String && b.K == K::String) {
                    if (op == '+')
                        return ConstValue::MakeString(a.S + b.S);
                    Fail(std::string("Operator '") + op + "' cannot be applied to strings");
                }

                if (!a.IsNumeric() || !b.IsNumeric()) {
                    Fail(std::string("Operator '") + op + "' cannot be applied to " + KindName(a.K) + " and " +
                         KindName(b.K));
                }

                // integer arithmetic (checked, truncating division like C++/AngelScript)
                if (a.K == K::Int && b.K == K::Int) {
                    const int64_t x = a.I;
                    const int64_t y = b.I;
                    constexpr int64_t kMax = std::numeric_limits<int64_t>::max();
                    constexpr int64_t kMin = std::numeric_limits<int64_t>::min();

                    switch (op) {
                    case '+':
                        if ((y > 0 && x > kMax - y) || (y < 0 && x < kMin - y))
                            Fail("Integer overflow in static_assert condition");
                        return ConstValue::MakeInt(x + y);
                    case '-':
                        if ((y < 0 && x > kMax + y) || (y > 0 && x < kMin + y))
                            Fail("Integer overflow in static_assert condition");
                        return ConstValue::MakeInt(x - y);
                    case '*': {
                        if (std::fabs(static_cast<long double>(x) * static_cast<long double>(y)) >=
                            9223372036854775808.0L)
                            Fail("Integer overflow in static_assert condition");
                        return ConstValue::MakeInt(x * y);
                    }
                    case '/':
                        if (y == 0)
                            Fail("Division by zero in static_assert condition");
                        if (x == kMin && y == -1)
                            Fail("Integer overflow in static_assert condition");
                        return ConstValue::MakeInt(x / y);
                    case '%':
                        if (y == 0)
                            Fail("Modulo by zero in static_assert condition");
                        if (y == -1)
                            return ConstValue::MakeInt(0);
                        return ConstValue::MakeInt(x % y);
                    }
                    Fail("Unknown operator");
                }

                // float arithmetic (any int operand is promoted)
                const double x = a.AsDouble();
                const double y = b.AsDouble();
                switch (op) {
                case '+': return ConstValue::MakeFloat(x + y);
                case '-': return ConstValue::MakeFloat(x - y);
                case '*': return ConstValue::MakeFloat(x * y);
                case '/':
                    if (y == 0.0)
                        Fail("Division by zero in static_assert condition");
                    return ConstValue::MakeFloat(x / y);
                case '%':
                    if (y == 0.0)
                        Fail("Modulo by zero in static_assert condition");
                    return ConstValue::MakeFloat(std::fmod(x, y));
                }
                Fail("Unknown operator");
            }

            template <typename T>
            static bool CompareOp(const std::string& op, const T& x, const T& y) {
                if (op == "==" || op == "=") return x == y;
                if (op == "!=") return x != y;
                if (op == "<")  return x < y;
                if (op == "<=") return x <= y;
                if (op == ">")  return x > y;
                return x >= y; // ">="
            }

            ConstValue Compare(const std::string& op, const ConstValue& a, const ConstValue& b) const {
                using K = ConstValue::Kind;

                if (a.K == K::String && b.K == K::String)
                    return ConstValue::MakeBool(CompareOp(op, a.S, b.S)); // byte-wise lexicographic

                if (a.K == K::Bool && b.K == K::Bool) {
                    if (op != "==" && op != "=" && op != "!=")
                        Fail("Operator '" + op + "' cannot be applied to bool operands");
                    return ConstValue::MakeBool(CompareOp(op, a.B, b.B));
                }

                if (a.IsNumeric() && b.IsNumeric()) {
                    if (a.K == K::Int && b.K == K::Int)
                        return ConstValue::MakeBool(CompareOp(op, a.I, b.I));
                    return ConstValue::MakeBool(CompareOp(op, a.AsDouble(), b.AsDouble()));
                }

                Fail(std::string("Cannot compare ") + KindName(a.K) + " with " + KindName(b.K));
            }

            // ---------------------------------------------------------------
            // Grammar
            // ---------------------------------------------------------------

            ConstValue ParseOr() {
                ConstValue l = ParseAnd();
                while (MatchOpOrWord("||", "or")) {
                    bool lb = RequireBool(l, "||");
                    ConstValue r = ParseAnd(); // no short-circuit; both sides are side-effect free
                    bool rb = RequireBool(r, "||");
                    l = ConstValue::MakeBool(lb || rb);
                }
                return l;
            }

            ConstValue ParseAnd() {
                ConstValue l = ParseEquality();
                while (MatchOpOrWord("&&", "and")) {
                    bool lb = RequireBool(l, "&&");
                    ConstValue r = ParseEquality();
                    bool rb = RequireBool(r, "&&");
                    l = ConstValue::MakeBool(lb && rb);
                }
                return l;
            }

            ConstValue ParseEquality() {
                ConstValue l = ParseRelational();
                for (;;) {
                    if (MatchOp("=="))
                        l = Compare("==", l, ParseRelational());
                    else if (MatchOp("!="))
                        l = Compare("!=", l, ParseRelational());
                    else
                        return l;
                }
            }

            ConstValue ParseRelational() {
                ConstValue l = ParseAdditive();
                for (;;) {
                    if (MatchOp("<="))      l = Compare("<=", l, ParseAdditive());
                    else if (MatchOp(">=")) l = Compare(">=", l, ParseAdditive());
                    else if (MatchOp("<"))  l = Compare("<", l, ParseAdditive());
                    else if (MatchOp(">"))  l = Compare(">", l, ParseAdditive());
                    else return l;
                }
            }

            ConstValue ParseAdditive() {
                ConstValue l = ParseMultiplicative();
                for (;;) {
                    if (MatchOp("+"))      l = Arith('+', l, ParseMultiplicative());
                    else if (MatchOp("-")) l = Arith('-', l, ParseMultiplicative());
                    else return l;
                }
            }

            ConstValue ParseMultiplicative() {
                ConstValue l = ParseUnary();
                for (;;) {
                    if (MatchOp("*"))      l = Arith('*', l, ParseUnary());
                    else if (MatchOp("/")) l = Arith('/', l, ParseUnary());
                    else if (MatchOp("%")) l = Arith('%', l, ParseUnary());
                    else return l;
                }
            }

            ConstValue ParseUnary() {
                if (MatchOpOrWord("!", "not")) {
                    ConstValue v = ParseUnary();
                    return ConstValue::MakeBool(!RequireBool(v, "!"));
                }
                if (MatchOp("-")) {
                    ConstValue v = ParseUnary();
                    if (v.K == ConstValue::Kind::Int) {
                        if (v.I == std::numeric_limits<int64_t>::min())
                            Fail("Integer overflow in static_assert condition");
                        return ConstValue::MakeInt(-v.I);
                    }
                    if (v.K == ConstValue::Kind::Float)
                        return ConstValue::MakeFloat(-v.F);
                    Fail(std::string("Unary '-' cannot be applied to ") + KindName(v.K));
                }
                if (MatchOp("+")) {
                    ConstValue v = ParseUnary();
                    if (!v.IsNumeric())
                        Fail(std::string("Unary '+' cannot be applied to ") + KindName(v.K));
                    return v;
                }
                return ParsePrimary();
            }

            ConstValue ParsePrimary() {
                if (mPos >= mToks.size())
                    Fail("Unexpected end of static_assert condition");

                const EvalTok& t = mToks[mPos];

                switch (t.Type) {
                case EvalTok::Kind::LParen: {
                    ++mPos;
                    ConstValue v = ParseOr();
                    if (mPos >= mToks.size() || mToks[mPos].Type != EvalTok::Kind::RParen)
                        Fail("Expected ')' in static_assert condition");
                    ++mPos;
                    return v;
                }
                case EvalTok::Kind::Number: {
                    ++mPos;
                    return ParseNumber(t.Text);
                }
                case EvalTok::Kind::String: {
                    ++mPos;
                    return ConstValue::MakeString(Unquote(t.Text));
                }
                case EvalTok::Kind::Ident: {
                    ++mPos;
                    if (t.Text == "true")
                        return ConstValue::MakeBool(true);
                    if (t.Text == "false")
                        return ConstValue::MakeBool(false);
                    if (mResolver) {
                        if (auto v = mResolver(t.Text))
                            return *v;
                    }
                    Fail("'" + t.Text + "' is not a compile-time constant");
                }
                default:
                    Fail("Unexpected '" + t.Text + "' in static_assert condition");
                }
            }

            ConstValue ParseNumber(std::string text) const {
                const bool hex = text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
                const bool bin = text.size() > 2 && text[0] == '0' && (text[1] == 'b' || text[1] == 'B');
                const bool oct = text.size() > 2 && text[0] == '0' && (text[1] == 'o' || text[1] == 'O');

                if (hex || bin || oct) {
                    const int base = hex ? 16 : bin ? 2 : 8;
                    errno = 0;
                    char* end = nullptr;
                    unsigned long long v = std::strtoull(text.c_str() + 2, &end, base);
                    if (end == text.c_str() + 2 || *end != '\0')
                        Fail("Invalid numeric literal '" + text + "' in static_assert condition");
                    if (errno == ERANGE || v > static_cast<unsigned long long>(std::numeric_limits<int64_t>::max()))
                        Fail("Integer literal '" + text + "' is out of range");
                    return ConstValue::MakeInt(static_cast<int64_t>(v));
                }

                // float if it has a '.', an exponent, or an 'f' suffix
                const bool is_float = text.find('.') != std::string::npos ||
                                      text.find('e') != std::string::npos || text.find('E') != std::string::npos ||
                                      (!text.empty() && (text.back() == 'f' || text.back() == 'F'));

                if (is_float) {
                    if (!text.empty() && (text.back() == 'f' || text.back() == 'F'))
                        text.pop_back();
                    char* end = nullptr;
                    double v = std::strtod(text.c_str(), &end);
                    if (end == text.c_str() || *end != '\0')
                        Fail("Invalid numeric literal '" + text + "' in static_assert condition");
                    return ConstValue::MakeFloat(v);
                }

                errno = 0;
                char* end = nullptr;
                unsigned long long v = std::strtoull(text.c_str(), &end, 10);
                if (end == text.c_str() || *end != '\0')
                    Fail("Invalid numeric literal '" + text + "' in static_assert condition");
                if (errno == ERANGE || v > static_cast<unsigned long long>(std::numeric_limits<int64_t>::max()))
                    Fail("Integer literal '" + text + "' is out of range");
                return ConstValue::MakeInt(static_cast<int64_t>(v));
            }

            // Strips surrounding quotes if the lexer left them on, and resolves
            // common escape sequences. If the lexer already did both, this is a no-op
            // apart from a string that itself starts and ends with a quote character.
            static std::string Unquote(const std::string& raw) {
                std::string s = raw;

                if (s.size() >= 6 && s.compare(0, 3, "\"\"\"") == 0 && s.compare(s.size() - 3, 3, "\"\"\"") == 0) {
                    return s.substr(3, s.size() - 6); // heredoc: no escapes
                }

                if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front())
                    s = s.substr(1, s.size() - 2);

                std::string out;
                out.reserve(s.size());
                for (size_t i = 0; i < s.size(); ++i) {
                    if (s[i] != '\\' || i + 1 >= s.size()) {
                        out += s[i];
                        continue;
                    }
                    char c = s[++i];
                    switch (c) {
                    case 'n':  out += '\n'; break;
                    case 't':  out += '\t'; break;
                    case 'r':  out += '\r'; break;
                    case '0':  out += '\0'; break;
                    case '\\': out += '\\'; break;
                    case '"':  out += '"'; break;
                    case '\'': out += '\''; break;
                    default:   out += '\\'; out += c; break;
                    }
                }
                return out;
            }
        };

        // Converts a constant's evaluated initializer to its declared type.
        // Returns nullopt if the declared type can't be a compile-time constant.
        std::optional<ConstValue> CoerceToDeclaredType(const AST::ASTTypeRef& type, ConstValue v,
                                                       const std::string& name, const SourceLocation& loc) {
            using K = ConstValue::Kind;

            if (type.IsHandle || type.IsReference || type.ArrayDepth > 0)
                return std::nullopt;

            if (type.IsAuto)
                return v;

            const std::string t = StripNamespace(type.Name);

            static const std::unordered_set<std::string> kInts = {"int",  "int8",  "int16",  "int32", "int64",
                                                                  "uint", "uint8", "uint16", "uint32", "uint64"};

            auto mismatch = [&]() -> std::optional<ConstValue> {
                throw Exceptions::SemanticError("Constant '" + name + "' of type '" + type.Name +
                                                    "' cannot be initialised with a " + KindName(v.K) +
                                                    " value",
                                                loc);
            };

            if (kInts.count(t)) {
                if (v.K == K::Int)
                    return v;
                if (v.K == K::Float)
                    return ConstValue::MakeInt(static_cast<int64_t>(v.F)); // implicit truncation
                return mismatch();
            }
            if (t == "float" || t == "double") {
                if (v.IsNumeric())
                    return ConstValue::MakeFloat(v.AsDouble());
                return mismatch();
            }
            if (t == "bool") {
                if (v.K == K::Bool)
                    return v;
                return mismatch();
            }
            if (t == "string") {
                if (v.K == K::String)
                    return v;
                return mismatch();
            }

            return std::nullopt; // some other type: not a supported constant
        }
    } // namespace
    void AddFunctionOverloads(std::vector<const Symbol*>& result, const SymbolTable& table, const std::string& name) {
        auto overloads = table.FindOverloads(name);

        for (const auto* symbol : overloads) {
            if (symbol->Kind == AST::ASTDeclaration::Kind::Function) {
                result.push_back(symbol);
            }
        }
    }

    // Symbol table functions
    bool SymbolTable::Declare(Symbol symbol) {
        auto& bucket = mSymbols[symbol.QualifiedName];

        if (symbol.Kind != AST::ASTDeclaration::Kind::Function) {
            if (!bucket.empty()) {
                return false; // globals/types/namespaces can't collide at all
            }
            bucket.push_back(std::move(symbol));
            return true;
        }

        // functions, collide only if InternalName matches (same signature)
        for (const auto& existing : bucket) {
            if (existing.InternalName == symbol.InternalName) {
                return false; // identical signature already delcared
            }
        }
        bucket.push_back(std::move(symbol));
        return true;
    }

    const Symbol* SymbolTable::Find(const std::string& qualified_name) const {
        auto it = mSymbols.find(qualified_name);

        if (it == mSymbols.end() || it->second.empty())
            return nullptr;

        if (it->second.size() > 1)
            return nullptr; // ambiguous overload set

        return &it->second.front();
    }

    std::vector<const Symbol*> SymbolTable::FindOverloads(const std::string& qualified_name) const {
        std::vector<const Symbol*> result;
        auto it = mSymbols.find(qualified_name);
        if (it == mSymbols.end()) {
            return result;
        }
        for (const auto& sym : it->second) {
            result.push_back(&sym);
        }
        return result;
    }

    // Symantic parser functions
    SymanticAnalyser::SymanticAnalyser(Preprocessor& processor,std::vector<std::filesystem::path> import_paths) : mImportPaths(import_paths),mPreprocessor(processor)  {}

    std::string SymanticAnalyser::GenerateSignatureHash(const AST::ASTFunction func) const {
        std::string signature;

        for (const auto& param : func.Parameters) {
            signature += "_" + param.Type.Name + (param.Type.IsConst ? "C" : "") + (param.Type.IsHandle ? "H" : "") +
                         (param.Type.IsReference ? "R" : "") + std::to_string(param.Type.ArrayDepth) + param.Direction;
        }

        return Utils::Hash2String(Utils::Hash64(signature));
    }

    std::string SymanticAnalyser::GenerateInternalName(const AST::ASTDeclaration& decl,
                                                       const std::string& namespace_path,
                                                       const std::string& module_hash) {
        std::string prefix;
        std::string suffix;

        std::string namespace_hash = Utils::Hash2String(Utils::Hash64(namespace_path));
        std::string symbol_hash = Utils::Hash2String(Utils::Hash64(decl.Name));

        switch (decl.Type) {
        case AST::ASTDeclaration::Kind::Function: {
            auto func = std::get<AST::ASTFunction>(decl.Data);

            std::string signature_hash = GenerateSignatureHash(func);

            prefix = kInternalFunctionPrefix;
            suffix = module_hash + "_" + namespace_hash + "_" + symbol_hash + "_" + signature_hash + "_";
            break;
        }

        case AST::ASTDeclaration::Kind::Global: {
            auto global = std::get<AST::ASTGlobal>(decl.Data);
            std::string type_hash = Utils::Hash2String(Utils::Hash64(global.Type.Name));

            prefix = kInternalGlobalPrefix;
            suffix = module_hash + "_" + namespace_hash + "_" + symbol_hash + "_" + type_hash;
            break;
        }

        case AST::ASTDeclaration::Kind::Type: {
            prefix = kInternalTypePrefix;
            suffix = module_hash + "_" + namespace_hash + "_" + symbol_hash;
            break;
        }

        case AST::ASTDeclaration::Kind::Namespace:
            // namespaces don't generate symbools directly
            return decl.Name;

        case AST::ASTDeclaration::Kind::Raw:
            return decl.Name;

        default:
            return decl.Name;
        }

        return prefix + suffix;
    }

    void SymanticAnalyser::VisitDeclaration(AST::ASTDeclaration& decl, const std::string& enclosing_namespace,
                                            const std::string& module_hash, const std::string& module_path) {
        if (decl.Type == AST::ASTDeclaration::Kind::Namespace) {
            auto ns = std::get<std::shared_ptr<AST::ASTNamespace>>(decl.Data);
            std::string nested_ns = enclosing_namespace.empty() ? ns->Name : enclosing_namespace + "::" + ns->Name;

            for (auto& child : ns->Declarations) {
                VisitDeclaration(child, nested_ns, module_hash, module_path);
            }
            return;
        }

        if (decl.Type == AST::ASTDeclaration::Kind::Raw) {
            return;
        }

        const std::string& full_namespace = enclosing_namespace;

        std::string qualified_name = full_namespace.empty() ? decl.Name : full_namespace + "::" + decl.Name;
        std::string internal_name = GenerateInternalName(decl, full_namespace, module_hash);

        Symbol symbol{decl.Type, qualified_name, internal_name, module_path, decl.Location, decl.Exported, {}};

        if (decl.Type == AST::ASTDeclaration::Kind::Function) {
            const auto& function = std::get<AST::ASTFunction>(decl.Data);

            symbol.Signature.ReturnType = function.ReturnType;

            for (const auto& parameter : function.Parameters) {
                symbol.Signature.Parameters.push_back(parameter.Type);
            }
        }

        if (!mModuleSymbols[module_path].Declare(symbol)) {
            throw Exceptions::SemanticError(
                "Redeclaration of '" + qualified_name + "'" +
                    (decl.Type == AST::ASTDeclaration::Kind::Function ? " with an identical parameter signature" : ""),
                decl.Location);
        }

        if (decl.Exported) {
            ExportInfo info{decl.Name, internal_name, decl.Type, full_namespace, module_path, {}};

            if (decl.Type == AST::ASTDeclaration::Kind::Function) {
                info.Signature = symbol.Signature;
            }

            mModuleExports[module_path].push_back(std::move(info));
        }
    }

    void SymanticAnalyser::CheckModule(AST::ASTModule& module, const std::string module_path,
                                      bool include_in_emission) {
        auto state = mAnalyzedModules.find(module_path);
        if (state != mAnalyzedModules.end()) {
            if (!state->second) {
                throw Exceptions::SemanticError("Circular import detected involving module '" + module_path + "'", {});
            }
            return; // already analysed
        }
        mAnalyzedModules[module_path] = false;

        std::string module_hash = Utils::Hash2String(AST::HashModule(module));

        for (auto& import : module.Imports) {
            std::string imported_path = Common::Import2Path(import, mImportPaths);

            // check if this is a file import (symbol present and path exists)
            if (import.Symbol.has_value() && !imported_path.empty() && std::filesystem::exists(imported_path)) {
                import.IsFileImport = true;
            }

            if (!mAnalyzedModules.count(imported_path)) {
                AST::ASTModule imported = LoadAndParseModule(imported_path);
                CheckModule(imported, imported_path);
            } else if (!mAnalyzedModules[imported_path]) {
                throw Exceptions::SemanticError(
                    "Circular import detected: '" + module_path + "' -> '" + imported_path + "'", import.Location);
            }

            const auto& imported_exports = mModuleExports[imported_path];

            if (import.IsUsing) {
                // using: bring symbols into scope )
                if (import.IsFileImport) {
                    // using tests::testing; (testing is a file  bring all exports from that file
                    for (const auto& exp : imported_exports) {
                        mModuleUsing[module_path].push_back(exp);
                    }
                } else if (import.Symbol.has_value()) {
                    // using module::symbol;
                    auto match = std::find_if(imported_exports.begin(), imported_exports.end(),
                                              [&](const ExportInfo& e) { return e.OriginalName == *import.Symbol; });
                    if (match == imported_exports.end()) {
                        throw Exceptions::SemanticError("Module '" + imported_path + "' has no exported symbol '" +
                                                            *import.Symbol + "'",
                                                        import.Location);
                    }
                    mModuleUsing[module_path].push_back(*match);
                } else {
                    // using module; bring all exports
                    for (const auto& exp : imported_exports) {
                        mModuleUsing[module_path].push_back(exp);
                    }
                }
            } else {
                // import: can export if marked
                if (import.IsFileImport) {
                    // import tests::foo; (foo is a file)  treat like whole-module import
                    if (import.Exported) {
                        for (const auto& exp : imported_exports) {
                            mModuleExports[module_path].push_back(exp);
                        }
                    }
                    // no symbol lookup; we just let the import exist for resolution
                } else if (import.Symbol.has_value()) {
                    // import module::symbol;
                    auto match = std::find_if(imported_exports.begin(), imported_exports.end(),
                                              [&](const ExportInfo& e) { return e.OriginalName == *import.Symbol; });
                    if (match == imported_exports.end()) {
                        throw Exceptions::SemanticError("Module '" + imported_path + "' has no exported symbol '" +
                                                            *import.Symbol + "'",
                                                        import.Location);
                    }
                    if (import.Exported) {
                        mModuleExports[module_path].push_back(*match);
                    }
                } else if (import.Exported) {
                    // export import module;
                    for (const auto& exp : imported_exports) {
                        mModuleExports[module_path].push_back(exp);
                    }
                }
            }
        }

        mParsedModules[module_path] = module;

        for (auto& decl : module.Declarations) {
            VisitDeclaration(decl, "", module_hash, module_path);
        }

        CheckAssertions(module, module_path);

        if (include_in_emission) {
            mEmissionOrder.push_back(module_path);
        }
        mAnalyzedModules[module_path] = true;
    }

    AST::ASTModule SymanticAnalyser::LoadAndParseModule(
        const std::string& module_path
    ) {
        std::string source = Common::GetScriptFromFS(module_path);
        std::string processed_source;

        if (!mPreprocessor.Process(source, processed_source)) {
            throw Exceptions::SemanticError(
                "Preprocessing failed for module '" + module_path + "'",
                {}
            );
        }

        mModuleSources[module_path] = processed_source;

        auto tokens = Lexer::Lex(processed_source, module_path);
        return Parser::ParseLexerOutput(tokens);
    }

    const Symbol* SymanticAnalyser::FindSymbol(const std::string& qualified_name,
                                               const std::string& module_path) const {
        if (!module_path.empty()) {
            auto module = mModuleSymbols.find(module_path);
            return module == mModuleSymbols.end() ? nullptr : module->second.Find(qualified_name);
        }

        for (const auto& path : mEmissionOrder) {
            auto module = mModuleSymbols.find(path);
            if (module != mModuleSymbols.end()) {
                if (const Symbol* symbol = module->second.Find(qualified_name)) {
                    return symbol;
                }
            }
        }
        return nullptr;
    }

    const Symbol* SymanticAnalyser::ResolveSymbol(const std::string& module_path,
                                                  const std::string& qualified_name) const {
        // first check local symbols
        if (const Symbol* local = FindSymbol(qualified_name, module_path)) {
            return local;
        }

        // check symbols brought in via 'using'
        auto using_it = mModuleUsing.find(module_path);
        if (using_it != mModuleUsing.end()) {
            for (const auto& export_info : using_it->second) {
                const std::string exported_name = export_info.Namespace.empty()
                                                      ? export_info.OriginalName
                                                      : export_info.Namespace + "::" + export_info.OriginalName;
                if (qualified_name == exported_name || qualified_name == export_info.OriginalName) {
                    if (const Symbol* symbol = FindSymbol(export_info.OriginalName, export_info.Modulepath)) {
                        return symbol;
                    }
                    if (const Symbol* symbol = FindSymbol(exported_name, export_info.Modulepath)) {
                        return symbol;
                    }
                }
            }
        }

        // then check imports
        auto parsed = mParsedModules.find(module_path);
        if (parsed == mParsedModules.end()) {
            return nullptr;
        }

        for (const auto& import : parsed->second.Imports) {
            if (import.IsUsing)
                continue; // already handled

            const std::string imported_path = Common::Import2Path(import, mImportPaths);
            auto exports = mModuleExports.find(imported_path);
            if (exports == mModuleExports.end()) {
                continue;
            }

            for (const ExportInfo& export_info : exports->second) {
                if (!import.IsFileImport && import.Symbol && export_info.OriginalName != *import.Symbol) {
                    continue;
                }

                const std::string exported_name = export_info.Namespace.empty()
                                                      ? export_info.OriginalName
                                                      : export_info.Namespace + "::" + export_info.OriginalName;

                // Build the full module path from import.Path
                std::string full_module_path;
                for (size_t i = 0; i < import.Path.size(); ++i) {
                    if (i > 0)
                        full_module_path += "::";
                    full_module_path += import.Path[i];
                }

                const std::string module_qualified_name = full_module_path + "::" + export_info.OriginalName;

                if (qualified_name == exported_name || qualified_name == export_info.OriginalName ||
                    qualified_name == module_qualified_name) {

                    if (const Symbol* symbol = FindSymbol(export_info.OriginalName, export_info.Modulepath)) {
                        return symbol;
                    }
                    if (const Symbol* symbol = FindSymbol(exported_name, export_info.Modulepath)) {
                        return symbol;
                    }
                }
            }
        }

        return nullptr;
    }

    std::vector<const Symbol*> SymanticAnalyser::ResolveOverloads(const std::string& module_path,
                                                                  const std::string& qualified_name) const {
        std::vector<const Symbol*> result;
        std::unordered_set<const Symbol*> seen; // prevent duplicates

        auto add = [&](const std::string& name, const std::string& path) {
            auto module = mModuleSymbols.find(path);

            if (module == mModuleSymbols.end())
                return;

            auto overloads = module->second.FindOverloads(name);

            for (const auto* symbol : overloads) {
                if (symbol->Kind == AST::ASTDeclaration::Kind::Function && seen.insert(symbol).second) {
                    result.push_back(symbol);
                }
            }
        };

        add(qualified_name, module_path);

        if (!result.empty())
            return result;

        auto using_it = mModuleUsing.find(module_path);

        if (using_it != mModuleUsing.end()) {
            for (const auto& export_info : using_it->second) {

                std::string exported_name = export_info.Namespace.empty()
                                                ? export_info.OriginalName
                                                : export_info.Namespace + "::" + export_info.OriginalName;
                if (qualified_name == export_info.OriginalName || qualified_name == exported_name) {
                    add(exported_name, export_info.Modulepath);
                }
            }
        }

        if (!result.empty())
            return result;

        auto parsed = mParsedModules.find(module_path);

        if (parsed == mParsedModules.end())
            return result;

        // debug loop show imports and exports
        for (const auto& import : parsed->second.Imports) {
            const std::string imported_path = Common::Import2Path(import, mImportPaths);

            CE_LOG(CECommon::LogLevel::Debug, "Import {} -> {}", import.Path.empty() ? "empty" : import.Path[0], imported_path);

            auto exports = mModuleExports.find(imported_path);

            if (exports == mModuleExports.end()) {
                CE_LOG(CECommon::LogLevel::Debug, "No exports found");
                continue;
            }

            for (const auto& e : exports->second) {
                CE_LOG(CECommon::LogLevel::Debug, "Export: {}::{}", e.Namespace, e.OriginalName);
            }
        }

        for (const auto& import : parsed->second.Imports) {
            if (import.IsUsing)
                continue;

            const std::string imported_path = Common::Import2Path(import, mImportPaths);

            auto exports = mModuleExports.find(imported_path);

            if (exports == mModuleExports.end())
                continue;

            for (const auto& export_info : exports->second) {
                if (export_info.Type != AST::ASTDeclaration::Kind::Function)
                    continue;

                if (!import.IsFileImport && import.Symbol && export_info.OriginalName != *import.Symbol) {
                    continue;
                }

                std::string exported_name = export_info.Namespace.empty()
                                                ? export_info.OriginalName
                                                : export_info.Namespace + "::" + export_info.OriginalName;

                std::string module_qualified_name;

                for (size_t i = 0; i < import.Path.size(); ++i) {
                    if (i > 0)
                        module_qualified_name += "::";

                    module_qualified_name += import.Path[i];
                }

                if (!module_qualified_name.empty())
                    module_qualified_name += "::";

                module_qualified_name += export_info.OriginalName;

                if (qualified_name == export_info.OriginalName || qualified_name == exported_name ||
                    qualified_name == module_qualified_name) {

                    CE_LOG(CECommon::LogLevel::Debug, "Found matching export, calling add({}, {})", exported_name,
                           export_info.Modulepath);

                    add(exported_name, export_info.Modulepath);

                    CE_LOG(CECommon::LogLevel::Debug, "After add, result size: {}", result.size());
                }
            }
        }

        return result;
    }

    const Symbol* SymanticAnalyser::ResolveFunction(const std::string& module_path, const std::string& name,
                                                    const std::vector<AST::ASTTypeRef>& arguments) const {
        auto overloads = ResolveOverloads(module_path, name);

        CE_LOG(CECommon::LogLevel::Debug, "[Semantic Parser] Resolving: {}", name);

        CE_LOG(CECommon::LogLevel::Debug, "Overloads found: {}", overloads.size());
        for (auto* x : overloads) {
            CE_LOG(CECommon::LogLevel::Debug, "candidate: {}", x->QualifiedName);
        }

        const Symbol* match = nullptr;

        for (const auto* symbol : overloads) {
            const auto& parameters = symbol->Signature.Parameters;

            if (parameters.size() != arguments.size())
                continue;

            bool valid = true;

            for (size_t i = 0; i < parameters.size(); ++i) {
                const auto& expected = parameters[i];
                const auto& actual = arguments[i];

                std::string expected_name = StripNamespace(expected.Name);
                std::string actual_name = StripNamespace(actual.Name);

                if (actual_name == "null") {
                    if (!expected.IsHandle) {
                        valid = false;
                        break;
                    }
                    continue; // skip rest of checks for null matching a handle
                }

                if (expected_name != actual_name) {
                    valid = false;
                    break;
                }

                if (expected.IsConst != actual.IsConst) {
                    valid = false;
                    break;
                }

                if (expected.IsHandle != actual.IsHandle) {
                    valid = false;
                    break;
                }

                if (expected.IsReference != actual.IsReference) {
                    valid = false;
                    break;
                }

                if (expected.ArrayDepth != actual.ArrayDepth) {
                    valid = false;
                    break;
                }
            }

            if (valid) {
                if (match != nullptr) {
                    // ambiguous overload
                    return nullptr;
                }

                match = symbol;
            }
        }

        return match;
    }

    AST::ASTTypeRef SymanticAnalyser::GetGlobalType(const Symbol* symbol) const {
        auto mod_it = mParsedModules.find(symbol->SourceModule);
        if (mod_it != mParsedModules.end()) {
            const auto* decl = FindDeclarationInModule(mod_it->second, symbol->QualifiedName);
            if (decl && decl->Type == AST::ASTDeclaration::Kind::Global) {
                const auto& global = std::get<AST::ASTGlobal>(decl->Data);
                return global.Type;
            }
        }
        return {};
    }

    AST::ASTTypeRef SymanticAnalyser::GetMemberType(const std::string& class_name, const std::string& member_name,
                                                    const std::string& module_path) const {
        // 1. Check hardcoded engine types first
        std::string clean_class = StripNamespace(class_name);
        if (clean_class == "Colour") {
            if (member_name == "r" || member_name == "g" || member_name == "b" || member_name == "a") {
                AST::ASTTypeRef t;
                t.Name = "uint8";
                return t;
            }
        } else if (clean_class == "Vec3") {
            if (member_name == "x" || member_name == "y" || member_name == "z") {
                AST::ASTTypeRef t;
                t.Name = "float";
                return t;
            }
        } else if (clean_class == "Transform3D") {
            if (member_name == "position" || member_name == "rotation" || member_name == "scale") {
                AST::ASTTypeRef t;
                t.Name = "CE::Graphics::ThreeD::Vec3";
                return t;
            }
        } else if (clean_class == "Camera3D") {
            if (member_name == "position" || member_name == "target" || member_name == "up") {
                AST::ASTTypeRef t;
                t.Name = "CE::Graphics::ThreeD::Vec3";
                return t;
            } else if (member_name == "fov" || member_name == "nearClip" || member_name == "farClip") {
                AST::ASTTypeRef t;
                t.Name = "float";
                return t;
            } else if (member_name == "projection") {
                AST::ASTTypeRef t;
                t.Name = "CE::Graphics::ThreeD::CameraProjection";
                return t;
            }
        } else if (clean_class == "AudioEffect") {
            if (member_name == "enabled") {
                AST::ASTTypeRef t;
                t.Name = "bool";
                return t;
            } else if (member_name == "type") {
                AST::ASTTypeRef t;
                t.Name = "CE::Audio::AudioEffectType";
                return t;
            } else if (member_name == "wetMix" || member_name == "roomSize" || member_name == "damping") {
                AST::ASTTypeRef t;
                t.Name = "float";
                return t;
            }
        }

        // 2. Check script-defined classes
        const Symbol* class_sym = ResolveSymbol(module_path, class_name);
        if (class_sym && class_sym->Kind == AST::ASTDeclaration::Kind::Type) {
            auto mod_it = mParsedModules.find(class_sym->SourceModule);
            if (mod_it != mParsedModules.end()) {
                const auto* decl = FindDeclarationInModule(mod_it->second, class_sym->QualifiedName);
                if (decl && decl->Type == AST::ASTDeclaration::Kind::Type) {
                    const auto& ast_type = std::get<AST::ASTType>(decl->Data);
                    const auto& tokens = ast_type.Body;
                    for (size_t i = 0; i < tokens.size(); ++i) {
                        size_t j = i;
                        bool is_const = false;
                        if (tokens[j].Type == Lexer::Token::TokenType::KeywordConst) {
                            is_const = true;
                            j++;
                        }
                        if (j < tokens.size() && tokens[j].Type == Lexer::Token::TokenType::Identifier) {
                            std::string type_name = tokens[j].Value;
                            j++;
                            while (j + 1 < tokens.size() &&
                                   tokens[j].Type == Lexer::Token::TokenType::ScopeResolution &&
                                   tokens[j + 1].Type == Lexer::Token::TokenType::Identifier) {
                                type_name += "::" + tokens[j + 1].Value;
                                j += 2;
                            }
                            bool is_handle = false;
                            bool is_ref = false;
                            uint32_t array_depth = 0;
                            while (j < tokens.size()) {
                                if (tokens[j].Type == Lexer::Token::TokenType::Handle) {
                                    is_handle = true;
                                    j++;
                                } else if (tokens[j].Type == Lexer::Token::TokenType::Reference) {
                                    is_ref = true;
                                    j++;
                                } else if (tokens[j].Type == Lexer::Token::TokenType::OpenBracket) {
                                    if (j + 1 < tokens.size() &&
                                        tokens[j + 1].Type == Lexer::Token::TokenType::CloseBracket) {
                                        array_depth++;
                                        j += 2;
                                    } else {
                                        break;
                                    }
                                } else {
                                    break;
                                }
                            }
                            if (j < tokens.size() && tokens[j].Type == Lexer::Token::TokenType::Identifier) {
                                std::string name = tokens[j].Value;
                                j++;
                                if (j < tokens.size() && (tokens[j].Type == Lexer::Token::TokenType::Semicolon ||
                                                          tokens[j].Type == Lexer::Token::TokenType::Assignment ||
                                                          tokens[j].Type == Lexer::Token::TokenType::OpenParen)) {
                                    if (tokens[j].Type != Lexer::Token::TokenType::OpenParen) {
                                        if (name == member_name) {
                                            AST::ASTTypeRef t;
                                            t.Name = type_name;
                                            t.IsConst = is_const;
                                            t.IsHandle = is_handle;
                                            t.IsReference = is_ref;
                                            t.ArrayDepth = array_depth;
                                            return t;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return {};
    }

    const Symbol* SymanticAnalyser::FindDeclarationSymbol(const std::string& qualified_name,
                                                          const std::string& module_path,
                                                          const AST::ASTDeclaration& decl) const {

        auto module = mModuleSymbols.find(module_path);

        if (module == mModuleSymbols.end())
            return nullptr;

        auto overloads = module->second.FindOverloads(qualified_name);

        if (decl.Type == AST::ASTDeclaration::Kind::Raw) {
            return nullptr;
        }

        if (decl.Type != AST::ASTDeclaration::Kind::Function) {
            for (const auto* symbol : overloads) {
                if (symbol->Kind == decl.Type)
                    return symbol;
            }

            return nullptr;
        }

        const auto& function = std::get<AST::ASTFunction>(decl.Data);

        std::string signature_hash = GenerateSignatureHash(function);

        for (const auto* symbol : overloads) {
            if (symbol->InternalName.find(signature_hash) != std::string::npos)
                return symbol;
        }

        return nullptr;
    }

    void SymanticAnalyser::CheckAssertions(const AST::ASTModule& module, const std::string& module_path) {
        using TT = Lexer::Token::TokenType;

        std::unordered_set<const Symbol*> visiting; // cycle guard: const A = B; const B = A;

        // Resolves `name` as seen from `from_module` to a const global's typed value.
        std::function<std::optional<ConstValue>(const std::string&, const std::string&, const SourceLocation&)>
            resolve_const;

        resolve_const = [&](const std::string& from_module, const std::string& name,
                            const SourceLocation& loc) -> std::optional<ConstValue> {
            const Symbol* sym = ResolveSymbol(from_module, name);
            if (!sym || sym->Kind != AST::ASTDeclaration::Kind::Global)
                return std::nullopt;

            auto mod = mParsedModules.find(sym->SourceModule);
            if (mod == mParsedModules.end())
                return std::nullopt;

            const auto* decl = FindDeclarationInModule(mod->second, sym->QualifiedName);
            if (!decl || decl->Type != AST::ASTDeclaration::Kind::Global)
                return std::nullopt;

            const auto& global = std::get<AST::ASTGlobal>(decl->Data);

            if (!global.Type.IsConst)
                throw Exceptions::SemanticError(
                    "'" + name + "' is not const, so it can't be used in a static_assert", loc);

            if (!visiting.insert(sym).second)
                throw Exceptions::SemanticError("Circular constant dependency involving '" + name + "'", loc);

            // Tolerate a leading '=' and trailing ';' in the stored initializer.
            std::vector<Lexer::Token> toks = global.Initializer;
            if (!toks.empty() && toks.front().Type == TT::Assignment)
                toks.erase(toks.begin());
            if (!toks.empty() && toks.back().Type == TT::Semicolon)
                toks.pop_back();

            if (toks.empty())
                throw Exceptions::SemanticError("Constant '" + name + "' has no initializer", loc);

            // Evaluate in the context of the module that defines the constant.
            const std::string defining_module = sym->SourceModule;
            ConstExprEvaluator inner(
                NormaliseTokens(toks),
                [&, defining_module](const std::string& n) { return resolve_const(defining_module, n, loc); }, loc);

            ConstValue raw = inner.Evaluate();
            visiting.erase(sym);

            auto coerced = CoerceToDeclaredType(global.Type, std::move(raw), name, loc);
            if (!coerced)
                throw Exceptions::SemanticError(
                    "Constant '" + name + "' has type '" + global.Type.Name +
                        "', which can't be used in a static_assert (only int, float, bool and string are supported)",
                    loc);
            return coerced;
        };

        for (const auto& assertion : module.Assertions) {
            const SourceLocation loc = assertion.Location;

            ConstExprEvaluator evaluator(
                NormaliseTokens(assertion.Condition),
                [&](const std::string& name) { return resolve_const(module_path, name, loc); }, loc);

            ConstValue result = evaluator.Evaluate();

            if (result.K != ConstValue::Kind::Bool) {
                throw Exceptions::SemanticError(
                    std::string("static_assert condition must be a bool expression, got ") + KindName(result.K),
                    loc);
            }

            if (!result.B) {
                std::string msg = "static_assert failed";
                if (!assertion.Message.empty())
                    msg += ": " + assertion.Message;
                throw Exceptions::SemanticError(msg, loc);
            }
        }
    }
} // namespace CE::Scripting::Impl::Semantics