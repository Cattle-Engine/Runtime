#!/usr/bin/env python3
"""
Stub AngelScript binding generator.

Recursively scans a directory for *.idl.yaml files and emits ONE .cpp/.h pair
that registers every enum / type / behaviour / method / operator / property /
function / constant / typedef with AngelScript so scripts compile and pass the
engine's declaration checks. All callables are bound to a single no-op
asCALL_GENERIC stub, so nothing is ever executed.

Usage:
  python stub_binding_gen.py --idl_dir path/to/idl \
      --output_header out/StubBindings.h --output_source out/StubBindings.cpp
"""
from __future__ import annotations

import argparse
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import yaml

# Optionally reuse your existing flag/behaviour tables if generator.py is importable.
try:
    import generator as _gen  # type: ignore
    _EXT_FLAGS: dict[str, str] = dict(getattr(_gen, "FLAG_MAP", {}))
    _EXT_BEHAVIOURS: dict[str, str] = dict(getattr(_gen, "BEHAVIOUR_MAP", {}))
except Exception:  # noqa: BLE001
    _EXT_FLAGS, _EXT_BEHAVIOURS = {}, {}

def _norm(name: str) -> str:
    return re.sub(r"[^a-z0-9]", "", name.lower())


_FLAGS: dict[str, str] = {
    "ref": "asOBJ_REF",
    "value": "asOBJ_VALUE",
    "gc": "asOBJ_GC",
    "pod": "asOBJ_POD",
    "nohandle": "asOBJ_NOHANDLE",
    "scoped": "asOBJ_SCOPED",
    "template": "asOBJ_TEMPLATE",
    "ashandle": "asOBJ_ASHANDLE",
    "nocount": "asOBJ_NOCOUNT",
    "implicithandle": "asOBJ_IMPLICIT_HANDLE",
    "appclass": "asOBJ_APP_CLASS",
    "appclassconstructor": "asOBJ_APP_CLASS_CONSTRUCTOR",
    "appclassdestructor": "asOBJ_APP_CLASS_DESTRUCTOR",
    "appclassassignment": "asOBJ_APP_CLASS_ASSIGNMENT",
    "appclasscopyconstructor": "asOBJ_APP_CLASS_COPY_CONSTRUCTOR",
    "appclassc": "asOBJ_APP_CLASS_C",
    "appclassd": "asOBJ_APP_CLASS_D",
    "appclassa": "asOBJ_APP_CLASS_A",
    "appclassk": "asOBJ_APP_CLASS_K",
    "appclassall": "asOBJ_APP_CLASS_CDAK",
    "appprimitive": "asOBJ_APP_PRIMITIVE",
    "appfloat": "asOBJ_APP_FLOAT",
    "apparray": "asOBJ_APP_ARRAY",
}

_BEHAVIOURS: dict[str, str] = {
    "construct": "asBEHAVE_CONSTRUCT",
    "constructor": "asBEHAVE_CONSTRUCT",
    "listconstruct": "asBEHAVE_LIST_CONSTRUCT",
    "destruct": "asBEHAVE_DESTRUCT",
    "destructor": "asBEHAVE_DESTRUCT",
    "factory": "asBEHAVE_FACTORY",
    "listfactory": "asBEHAVE_LIST_FACTORY",
    "addref": "asBEHAVE_ADDREF",
    "release": "asBEHAVE_RELEASE",
    "getweakrefflag": "asBEHAVE_GET_WEAKREF_FLAG",
    "templatecallback": "asBEHAVE_TEMPLATE_CALLBACK",
    "getrefcount": "asBEHAVE_GETREFCOUNT",
    "setgcflag": "asBEHAVE_SETGCFLAG",
    "getgcflag": "asBEHAVE_GETGCFLAG",
    "enumrefs": "asBEHAVE_ENUMREFS",
    "releaserefs": "asBEHAVE_RELEASEREFS",
}

_FIXED_BEHAVIOUR_DECL: dict[str, str] = {
    "asBEHAVE_DESTRUCT": "void f()",
    "asBEHAVE_ADDREF": "void f()",
    "asBEHAVE_RELEASE": "void f()",
    "asBEHAVE_SETGCFLAG": "void f()",
    "asBEHAVE_GETGCFLAG": "bool f()",
    "asBEHAVE_GETREFCOUNT": "int f()",
    "asBEHAVE_ENUMREFS": "void f(int&in)",
    "asBEHAVE_RELEASEREFS": "void f(int&in)",
    "asBEHAVE_TEMPLATE_CALLBACK": "bool f(int&in, bool&out)",
}

_OPERATOR_NAMES: dict[str, str] = {
    "+": "opAdd", "-": "opSub", "*": "opMul", "/": "opDiv", "%": "opMod",
    "&": "opAnd", "|": "opOr", "^": "opXor", "<<": "opShl", ">>": "opShr",
    ">>>": "opShrU", "**": "opPow",
    "==": "opEquals", "=": "opAssign",
    "+=": "opAddAssign", "-=": "opSubAssign", "*=": "opMulAssign",
    "/=": "opDivAssign", "%=": "opModAssign", "&=": "opAndAssign",
    "|=": "opOrAssign", "^=": "opXorAssign", "<<=": "opShlAssign",
    ">>=": "opShrAssign",
    "[]": "opIndex", "()": "opCall",
    "<": "opCmp", "<=": "opCmp", ">": "opCmp", ">=": "opCmp",
    "~": "opCom", "++": "opPreInc", "--": "opPreDec",
}

_PRIMITIVES: dict[str, str] = {
    "bool": "bool",
    "int8": "int8_t", "int16": "int16_t", "int": "int32_t", "int32": "int32_t", "int64": "int64_t",
    "uint8": "uint8_t", "uint16": "uint16_t", "uint": "uint32_t", "uint32": "uint32_t", "uint64": "uint64_t",
    "float": "float", "double": "double",
}


def resolve_flag(name: str) -> str | None:
    if name in _EXT_FLAGS:
        return _EXT_FLAGS[name]
    key = _norm(name)
    if key.startswith("asobj"):
        key = key[len("asobj"):]
    return _FLAGS.get(key)


def resolve_behaviour(name: str) -> str | None:
    if name in _EXT_BEHAVIOURS:
        return _EXT_BEHAVIOURS[name]
    key = _norm(name)
    if key.startswith("asbehave"):
        key = key[len("asbehave"):]
    return _BEHAVIOURS.get(key)


def signature_parts(signature: str) -> list[str]:
    """Split a parameter list on top-level commas only."""
    signature = (signature or "").strip()
    if not signature:
        return []
    parts: list[str] = []
    start = 0
    depth = {"<": 0, "(": 0, "[": 0}
    closer = {">": "<", ")": "(", "]": "["}
    for i, ch in enumerate(signature):
        if ch in depth:
            depth[ch] += 1
        elif ch in closer:
            depth[closer[ch]] = max(0, depth[closer[ch]] - 1)
        elif ch == "," and not any(depth.values()):
            part = signature[start:i].strip()
            if part:
                parts.append(part)
            start = i + 1
    tail = signature[start:].strip()
    if tail:
        parts.append(tail)
    return parts


def reg_signature(signature: str) -> str:
    return ", ".join(signature_parts(signature))


def esc(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def sanitize(value: str) -> str:
    out = re.sub(r"[^A-Za-z0-9]+", "_", value).strip("_")
    return out or "Symbol"


class Writer:
    def __init__(self) -> None:
        self.lines: list[str] = []
        self.level = 0
        self.ns: str | None = None  # AS default namespace currently set

    def w(self, text: str = "") -> None:
        self.lines.append(("    " * self.level + text) if text else "")

    def set_ns(self, ns: str) -> None:
        if self.ns != ns:
            self.w(f'engine->SetDefaultNamespace("{esc(ns)}");')
            self.ns = ns

    def check(self, call: str) -> None:
        self.w(f"CE_STUB_CHECK({call});")

    def empty(self) -> bool:
        return not any(line.strip() for line in self.lines)


@dataclass
class Doc:
    path: Path
    data: dict[str, Any]
    ns: str
    priority: int
    index: int = 0

    @property
    def tag(self) -> str:
        return f"{self.index}_{sanitize(self.path.name.replace('.idl.yaml', ''))}"


@dataclass
class Options:
    real_types: bool
    value_size: int

def render_flags(flags: list[str], cpp_type: str, opts: Options) -> tuple[str, bool]:
    """Returns (flags expression, is_ref_type)."""
    parts: list[str] = []
    auto = False
    for flag in flags:
        if flag == "AutoGetFlags":
            auto = True
            continue
        # The dummy compiler does not know the native value type's size.
        # asOBJ_APP_CLASS_ALLINTS validates that size, so retain only the
        # script-visible value/POD declaration in this mode.
        if not opts.real_types and flag == "AppClassAllInts":
            continue
        resolved = resolve_flag(flag)
        if resolved is None:
            raise ValueError(f"unknown flag '{flag}'")
        parts.append(resolved)

    joined = " | ".join(parts)
    is_ref = "asOBJ_REF" in joined
    if auto:
        if opts.real_types:
            parts.append(f"asGetTypeTraits<{cpp_type}>()")
        else:
            # We don't have the C++ type; treat it as a plain value type.
            if not is_ref and "asOBJ_VALUE" not in joined:
                parts.append("asOBJ_VALUE")
            if not is_ref and "asOBJ_POD" not in joined:
                parts.append("asOBJ_POD")
    return (" | ".join(parts) or "0"), is_ref


def type_size(is_ref: bool, cpp_type: str, opts: Options) -> str:
    if is_ref:
        return "0"
    return f"sizeof({cpp_type})" if opts.real_types else str(opts.value_size)


def validate(doc: Doc, opts: Options) -> list[str]:
    errs: list[str] = []
    p = doc.path.as_posix()
    d = doc.data

    def need(obj: dict, key: str, ctx: str) -> None:
        if key not in obj or obj[key] in (None, ""):
            errs.append(f"{p}: {ctx} missing '{key}'")

    for t in d.get("ASTypes", []):
        need(t, "Name", "ASType")
        tn = t.get("Name", "?")
        if opts.real_types:
            need(t, "CppType", f"ASType '{tn}'")
        flags = list(t.get("Flags", t.get("ASFlags", [])))
        if not flags:
            errs.append(f"{p}: ASType '{tn}' missing Flags")
        try:
            render_flags(flags, t.get("CppType", ""), opts)
        except ValueError as e:
            errs.append(f"{p}: ASType '{tn}': {e}")
        for b in t.get("Behaviours", []):
            if "Type" not in b:
                errs.append(f"{p}: behaviour in '{tn}' missing 'Type'")
            elif resolve_behaviour(b["Type"]) is None:
                errs.append(f"{p}: behaviour '{b['Type']}' in '{tn}' is not supported")
        for m in t.get("Methods", []):
            need(m, "Name", f"method in '{tn}'")
            need(m, "ReturnType", f"method '{m.get('Name', '?')}' in '{tn}'")
        for o in t.get("Operators", []):
            need(o, "Operator", f"operator in '{tn}'")
            need(o, "ReturnType", f"operator '{o.get('Operator', '?')}' in '{tn}'")
        for pr in t.get("Properties", []):
            need(pr, "Type", f"property in '{tn}'")
            need(pr, "ASMember", f"property in '{tn}'")
            if opts.real_types:
                need(pr, "CppMember", f"property in '{tn}'")
    for f in d.get("ASFunctions", []):
        need(f, "Name", "ASFunction")
        need(f, "ReturnType", f"function '{f.get('Name', '?')}'")
    for e in d.get("ASEnums", []):
        need(e, "Name", "ASEnum")
        if not e.get("Values"):
            errs.append(f"{p}: enum '{e.get('Name', '?')}' has no values")
    for c in d.get("ASConstants", []):
        need(c, "Name", "ASConstant")
        need(c, "Type", f"constant '{c.get('Name', '?')}'")
        if "Value" not in c:
            errs.append(f"{p}: constant '{c.get('Name', '?')}' missing 'Value'")
    for a in d.get("ASTypeAliases", []):
        need(a, "Name", "ASTypeAlias")
        need(a, "Target", f"alias '{a.get('Name', '?')}'")
    for cf in d.get("ASClassFunctions", []):
        need(cf, "Name", "ASClassFunction")
        need(cf, "ReturnType", f"class function '{cf.get('Name', '?')}'")
    return errs


def validate_global(docs: list[Doc]) -> list[str]:
    """Cross-file duplicate checks (AngelScript would reject these at registration)."""
    errs: list[str] = []
    seen: dict[tuple[str, str, str], str] = {}

    def track(kind: str, ns: str, name: str, path: Path) -> None:
        key = (kind, ns, name)
        if key in seen:
            errs.append(
                f"Duplicate {kind} '{ns + '::' if ns else ''}{name}' in {path.as_posix()} "
                f"(first defined in {seen[key]})"
            )
        else:
            seen[key] = path.as_posix()

    for doc in docs:
        for t in doc.data.get("ASTypes", []):
            track("ASType", t.get("Namespace", doc.ns), t.get("Name", ""), doc.path)
        for e in doc.data.get("ASEnums", []):
            track("ASEnum", e.get("Namespace", doc.ns), e.get("Name", ""), doc.path)
        for a in doc.data.get("ASTypeAliases", []):
            track("ASTypeAlias", a.get("Namespace", doc.ns), a.get("Name", ""), doc.path)
    return errs

STUB = "CE_StubGeneric"


def phase1(doc: Doc, w: Writer, opts: Options) -> None:
    for interface in doc.data.get("ASInterfaces", []):
        w.set_ns(interface.get("Namespace", doc.ns))
        w.check(f'engine->RegisterInterface("{esc(interface["Name"])}")')

    for e in doc.data.get("ASEnums", []):
        ns = e.get("Namespace", doc.ns)
        w.set_ns(ns)
        w.check(f'engine->RegisterEnum("{esc(e["Name"])}")')
        cpp_type = e.get("CppType", "")
        counter = 0
        for raw in e.get("Values", []):
            raw = str(raw)
            name, expr = raw, None
            if "=" in raw:
                name, expr = (s.strip() for s in raw.split("=", 1))
            if opts.real_types:
                cpp_val = f"{cpp_type}::{name}" if cpp_type else name
                value_expr = f"(int){cpp_val}"
            else:
                if expr is not None:
                    try:
                        counter = int(expr, 0)
                    except ValueError:
                        pass  # keep running counter if it isn't a plain literal
                value_expr = str(counter)
                counter += 1
            w.check(f'engine->RegisterEnumValue("{esc(e["Name"])}", "{esc(name)}", {value_expr})')

    for t in doc.data.get("ASTypes", []):
        w.set_ns(t.get("Namespace", doc.ns))
        cpp_type = t.get("CppType", "")
        flags, is_ref = render_flags(list(t.get("Flags", t.get("ASFlags", []))), cpp_type, opts)
        w.check(
            f'engine->RegisterObjectType("{esc(t["Name"])}", '
            f"{type_size(is_ref, cpp_type, opts)}, {flags})"
        )


def phase2(doc: Doc, w: Writer, opts: Options) -> None:
    for a in doc.data.get("ASTypeAliases", []):
        w.set_ns(a.get("Namespace", doc.ns))
        w.check(f'engine->RegisterTypedef("{esc(a["Name"])}", "{esc(a["Target"])}")')

    # Runtime bindings may register std::vector wrappers from an ASType's
    # AfterRegister snippet.  The script compiler needs the same script-side
    # declarations so properties such as Model.materials type-check correctly.
    # All object types have been registered before this phase runs.
    vector_pattern = re.compile(
        r'RegisterVector\s*<[^>]+>\s*\(\s*&mScriptEngine\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\)'
    )
    for t in doc.data.get("ASTypes", []):
        after_register = t.get("AfterRegister", "")
        for vector_name, element_name in vector_pattern.findall(after_register):
            type_namespace = t.get("Namespace", doc.ns)
            if "::" not in element_name and type_namespace:
                element_name = f"{type_namespace}::{element_name}"
            w.set_ns("CE::Containers")
            w.check(
                f'CE_RegisterStubVector(engine, "{esc(vector_name)}", "{esc(element_name)}")'
            )


def behaviour_decl(type_name: str, behaviour_macro: str, params: str) -> str:
    if behaviour_macro in _FIXED_BEHAVIOUR_DECL:
        return _FIXED_BEHAVIOUR_DECL[behaviour_macro]
    if behaviour_macro in ("asBEHAVE_FACTORY", "asBEHAVE_LIST_FACTORY"):
        return f"{type_name}@ f({params})"
    return f"void f({params})"


def operator_name(op: str, params: str) -> str:
    if op == "-" and not params:
        return "opNeg"
    return _OPERATOR_NAMES.get(op, op)  # unknown -> pass through (e.g. "opNeg")


def phase3(doc: Doc, w: Writer, opts: Options) -> None:
    for interface in doc.data.get("ASInterfaces", []):
        w.set_ns(interface.get("Namespace", doc.ns))
        for method in interface.get("Methods", []):
            w.check(
                f'engine->RegisterInterfaceMethod("{esc(interface["Name"])}", "{esc(method)}")'
            )

    for t in doc.data.get("ASTypes", []):
        ns = t.get("Namespace", doc.ns)
        name = esc(t["Name"])
        cpp_type = t.get("CppType", "")
        w.set_ns(ns)

        for c in t.get("Constructors", []):
            sig = reg_signature(c.get("Signature", ""))
            w.check(
                f'engine->RegisterObjectBehaviour("{name}", asBEHAVE_CONSTRUCT, '
                f'"void f({esc(sig)})", asFUNCTION({STUB}), asCALL_GENERIC)'
            )

        for b in t.get("Behaviours", []):
            macro = resolve_behaviour(b["Type"])
            decl = behaviour_decl(t["Name"], macro, reg_signature(b.get("Signature", "")))
            w.check(
                f'engine->RegisterObjectBehaviour("{name}", {macro}, '
                f'"{esc(decl)}", asFUNCTION({STUB}), asCALL_GENERIC)'
            )

        for m in t.get("Methods", []):
            decl = f'{m["ReturnType"]} {m["Name"]}({reg_signature(m.get("Signature", ""))})'
            if m.get("IsConst", False):
                decl += " const"
            w.check(
                f'engine->RegisterObjectMethod("{name}", "{esc(decl)}", '
                f"asFUNCTION({STUB}), asCALL_GENERIC)"
            )

        for o in t.get("Operators", []):
            params = reg_signature(o.get("Signature", ""))
            decl = f'{o["ReturnType"]} {operator_name(o["Operator"], params)}({params})'
            if o.get("IsConst", True):
                decl += " const"
            w.check(
                f'engine->RegisterObjectMethod("{name}", "{esc(decl)}", '
                f"asFUNCTION({STUB}), asCALL_GENERIC)"
            )

        for p in t.get("Properties", []):
            offset = f"asOFFSET({cpp_type}, {p['CppMember']})" if opts.real_types else "0"
            w.check(
                f'engine->RegisterObjectProperty("{name}", '
                f'"{esc(p["Type"])} {esc(p["ASMember"])}", {offset})'
            )


_const_counter = 0


def phase4(doc: Doc, w: Writer, opts: Options) -> None:
    global _const_counter

    # Functions implemented by a hand-written runtime binding. They still need
    # declarations in the standalone script compiler, whose bindings are
    # intentionally no-op stubs.
    for f in doc.data.get("ASExternalFunctions", []):
        w.set_ns(f.get("Namespace", doc.ns))
        decl = f'{f["ReturnType"]} {f["Name"]}({reg_signature(f.get("Signature", ""))})'
        w.check(
            f'engine->RegisterGlobalFunction("{esc(decl)}", asFUNCTION({STUB}), asCALL_GENERIC)'
        )

    for f in doc.data.get("ASFunctions", []):
        w.set_ns(f.get("Namespace", doc.ns))
        decl = f'{f["ReturnType"]} {f["Name"]}({reg_signature(f.get("Signature", ""))})'
        w.check(
            f'engine->RegisterGlobalFunction("{esc(decl)}", asFUNCTION({STUB}), asCALL_GENERIC)'
        )

    for cf in doc.data.get("ASClassFunctions", []):
        w.set_ns(cf.get("Namespace", doc.ns))
        decl = f'{cf["ReturnType"]} {cf["Name"]}({reg_signature(cf.get("Signature", ""))})'
        w.check(
            f'engine->RegisterGlobalFunction("{esc(decl)}", asFUNCTION({STUB}), asCALL_GENERIC)'
        )

    for c in doc.data.get("ASConstants", []):
        w.set_ns(c.get("Namespace", doc.ns))
        _const_counter += 1
        var = f"s_const_{_const_counter}_{sanitize(c['Name'])}"
        as_type, value = c["Type"], c["Value"]

        if opts.real_types:
            cpp_type = c.get("CppType") or as_type
            literal = f'"{esc(value)}"' if isinstance(value, str) else (
                ("true" if value else "false") if isinstance(value, bool) else str(value)
            )
            w.w(f"static const {cpp_type} {var} = {literal};")
        elif as_type in _PRIMITIVES:
            if isinstance(value, bool):
                literal = "true" if value else "false"
            else:
                literal = str(value)
                if as_type == "float" and not literal.endswith("f"):
                    literal += "f"
            w.w(f"static const {_PRIMITIVES[as_type]} {var} = {literal};")
        else:
            # Non-primitive (string, vec3, ...): zeroed placeholder storage, never read.
            w.w(f"alignas(16) static unsigned char {var}[256] = {{}};")
        w.check(
            f'engine->RegisterGlobalProperty("const {esc(as_type)} {esc(c["Name"])}", '
            f"(void*)&{var})"
        )


PHASES = [("Types", phase1), ("Typedefs", phase2), ("Members", phase3), ("Functions", phase4)]


def generate_source(docs: list[Doc], header_include: str, func_name: str, opts: Options) -> str:
    out = Writer()
    out.w("// GENERATED BY stub_binding_gen.py, DO NOT MODIFY")
    out.w("// Declaration-only AngelScript bindings: every callable is a no-op stub.")
    out.w("#include <angelscript.h>")
    out.w("#include <cstdint>")
    out.w("#include <cstdio>")
    out.w("#include <string>")

    if opts.real_types:
        headers: list[str] = []
        for doc in docs:
            for h in doc.data.get("RequiredCppHeaders", []):
                if h not in headers:
                    headers.append(h)
        if any("glm/gtx/" in h for h in headers):
            out.w("#define GLM_ENABLE_EXPERIMENTAL")
        out.w(f'#include "{header_include}"')
        for h in headers:
            h = h.strip()
            out.w(f"#include {h}" if h[:1] in "<\"" else f'#include "{h}"')
    else:
        out.w(f'#include "{header_include}"')

    out.w()
    out.w("#define CE_STUB_CHECK(expr) \\")
    out.w("    do { \\")
    out.w("        const int ce_r_ = (expr); \\")
    out.w("        if (ce_r_ < 0) { \\")
    out.w('            std::fprintf(stderr, "[AS stub bindings] %s failed (%d) at %s:%d\\n", #expr, ce_r_, __FILE__, __LINE__); \\')
    out.w("            return false; \\")
    out.w("        } \\")
    out.w("    } while (0)")
    out.w()
    out.w("namespace")
    out.w("{")
    out.w(f"void {STUB}(asIScriptGeneric*) {{}}")
    out.w()
    out.w("bool CE_RegisterStubVector(asIScriptEngine* engine, const char* vectorName, const char* elementName)")
    out.w("{")
    out.w("    const int typeResult = engine->RegisterObjectType(vectorName, 64, asOBJ_VALUE | asOBJ_POD);")
    out.w("    if (typeResult < 0) return false;")
    out.w("    const std::string element(elementName);")
    out.w("    const std::string index = element + \" &opIndex(uint)\";")
    out.w("    const std::string constIndex = \"const \" + element + \" &opIndex(uint) const\";")
    out.w("    if (engine->RegisterObjectMethod(vectorName, index.c_str(), asFUNCTION(CE_StubGeneric), asCALL_GENERIC) < 0) return false;")
    out.w("    if (engine->RegisterObjectMethod(vectorName, constIndex.c_str(), asFUNCTION(CE_StubGeneric), asCALL_GENERIC) < 0) return false;")
    out.w("    if (engine->RegisterObjectMethod(vectorName, \"uint size() const\", asFUNCTION(CE_StubGeneric), asCALL_GENERIC) < 0) return false;")
    out.w("    return true;")
    out.w("}")
    out.w()

    calls: list[list[str]] = [[] for _ in PHASES]
    for pi, (pname, fn) in enumerate(PHASES):
        for doc in docs:
            body = Writer()
            body.level = 1
            fn(doc, body, opts)
            if body.empty():
                continue
            fname = f"Register{pname}_{doc.tag}"
            calls[pi].append(fname)
            out.w(f"// {doc.path.as_posix()}")
            out.w(f"bool {fname}(asIScriptEngine* engine)")
            out.w("{")
            out.lines.extend(body.lines)
            out.w('    engine->SetDefaultNamespace("");')
            out.w("    return true;")
            out.w("}")
            out.w()
    out.w("} // namespace")
    out.w()
    out.w(f"bool {func_name}(asIScriptEngine* engine)")
    out.w("{")
    for pi, (pname, _) in enumerate(PHASES):
        for fname in calls[pi]:
            out.w(f"    if (!{fname}(engine)) return false;")
    out.w('    engine->SetDefaultNamespace("");')
    out.w("    return true;")
    out.w("}")
    out.w()
    return "\n".join(out.lines)


def generate_header(func_name: str) -> str:
    return "\n".join([
        "// GENERATED BY stub_binding_gen.py, DO NOT MODIFY",
        "#pragma once",
        "#include <angelscript.h>",
        "",
        "/// Registers declaration-only (no-op) bindings for every *.idl.yaml found.",
        f"bool {func_name}(asIScriptEngine* engine);",
        "",
    ])


def load_docs(root: Path, pattern: str) -> tuple[list[Doc], list[str]]:
    docs: list[Doc] = []
    errors: list[str] = []
    for path in sorted(root.rglob(pattern)):
        if not path.is_file():
            continue
        try:
            with path.open(encoding="utf-8") as fh:
                data = yaml.safe_load(fh) or {}
        except (OSError, yaml.YAMLError) as e:
            errors.append(f"{path.as_posix()}: failed to read/parse: {e}")
            continue
        if not isinstance(data, dict):
            errors.append(f"{path.as_posix()}: top level must be a mapping")
            continue
        docs.append(Doc(path, data, data.get("ASNamespace", ""), int(data.get("RegistrationPriority", 0))))


    docs.sort(key=lambda d: (d.priority, d.path.as_posix()))
    for i, d in enumerate(docs):
        d.index = i
    return docs, errors


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate declaration-only (stub) AngelScript bindings from *.idl.yaml files")
    ap.add_argument("--idl_dir", type=Path, required=True, help="Directory to search recursively")
    ap.add_argument("--output_header", type=Path, required=True)
    ap.add_argument("--output_source", type=Path, required=True)
    ap.add_argument("--function_name", default="RegisterStubBindings", help="Name of the generated register function")
    ap.add_argument("--pattern", default="*.idl.yaml", help="Glob used for discovery")
    ap.add_argument("--use_cpp_types", action="store_true",
                    help="Include RequiredCppHeaders and use real sizeof/asGetTypeTraits/asOFFSET/enum values "
                         "(needed if compiled bytecode must match the real engine)")
    ap.add_argument("--value_size", type=int, default=64, help="Placeholder size for value types in stub mode")
    args = ap.parse_args()

    if not args.idl_dir.is_dir():
        print(f"[Fatal] Not a directory: {args.idl_dir}", file=sys.stderr)
        return 1

    opts = Options(real_types=args.use_cpp_types, value_size=args.value_size)
    docs, errors = load_docs(args.idl_dir, args.pattern)
    if not docs and not errors:
        errors.append(f"No files matching '{args.pattern}' found under {args.idl_dir}")

    for doc in docs:
        errors.extend(validate(doc, opts))
    errors.extend(validate_global(docs))

    if errors:
        for e in errors:
            print(f"[Fatal] {e}", file=sys.stderr)
        return 1

    include = Path(os.path.relpath(args.output_header, start=args.output_source.parent)).as_posix()
    source = generate_source(docs, include, args.function_name, opts)
    header = generate_header(args.function_name)

    args.output_source.parent.mkdir(parents=True, exist_ok=True)
    args.output_header.parent.mkdir(parents=True, exist_ok=True)
    args.output_source.write_text(source, encoding="utf-8")
    args.output_header.write_text(header, encoding="utf-8")

    print(f"[Info] {len(docs)} IDL file(s) -> {args.output_source} / {args.output_header}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
