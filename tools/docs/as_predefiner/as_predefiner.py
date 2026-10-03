#!/usr/bin/env python3
from __future__ import annotations

import argparse
from collections import defaultdict
from pathlib import Path
from typing import Any, Iterable

import yaml


ROOT = Path(__file__).resolve().parents[3]
DEFAULT_IDL_DIRECTORY = ROOT / "source" / "engine" / "scripting" / "bindings"
DEFAULT_OUTPUT = ROOT / "as.predefined"

OPERATOR_NAMES = {
    "+": "opAdd",
    "-": "opSub",
    "*": "opMul",
    "/": "opDiv",
    "%": "opMod",
    "==": "opEquals",
    "=": "opAssign",
    "+=": "opAddAssign",
    "-=": "opSubAssign",
    "*=": "opMulAssign",
    "/=": "opDivAssign",
    "[]": "opIndex",
    "()": "opCall",
    "<": "opCmp",
    "<=": "opCmp",
    ">": "opCmp",
    ">=": "opCmp",
}


def declaration(return_type: str, name: str, signature: str, is_const: bool = False) -> str:
    result = f"{return_type} {name}({signature.strip()})"
    return f"{result} const" if is_const else result


def namespace_for(item: dict[str, Any], default: str) -> str:
    return str(item.get("Namespace", default))


def type_members(as_type: dict[str, Any]) -> list[str]:
    """Render declarations that AngelScript registers for one object type."""
    name = str(as_type["Name"])
    members: list[str] = []

    # Constructors and behaviours are registered with AngelScript as ``f``.
    # Keeping that name matches asIScriptFunction::GetDeclaration(), which is
    # the runtime generator this tool is based on.
    for constructor in as_type.get("Constructors", []):
        members.append(declaration("void", "f", str(constructor.get("Signature", ""))) + ";")

    for behaviour in as_type.get("Behaviours", []):
        behaviour_type = str(behaviour.get("Type", ""))
        signature = str(behaviour.get("Signature", ""))
        if behaviour_type == "Factory":
            members.append(declaration(f"{name}@", "f", signature) + ";")
        else:
            members.append(declaration("void", "f", signature) + ";")

    for method in as_type.get("Methods", []):
        members.append(
            declaration(
                str(method["ReturnType"]),
                str(method["Name"]),
                str(method.get("Signature", "")),
                bool(method.get("IsConst", False)),
            )
            + ";"
        )

    for operator in as_type.get("Operators", []):
        members.append(
            declaration(
                str(operator["ReturnType"]),
                OPERATOR_NAMES.get(str(operator["Operator"]), str(operator["Operator"])),
                str(operator.get("Signature", "")),
                bool(operator.get("IsConst", True)),
            )
            + ";"
        )

    for property_info in as_type.get("Properties", []):
        members.append(f"{property_info['Type']} {property_info['ASMember']};")

    return members


def collect_declarations(idl_files: Iterable[Path]) -> dict[str, list[str]]:
    """Collect predefinition declarations grouped by their AngelScript namespace."""
    by_namespace: dict[str, list[str]] = defaultdict(list)

    for idl_file in sorted(idl_files):
        with idl_file.open(encoding="utf-8") as source:
            data = yaml.safe_load(source) or {}
        default_namespace = str(data.get("ASNamespace", ""))

        for item in data.get("ASDeclarations", []):
            by_namespace[namespace_for(item, default_namespace)].append(f"{item['Name']};")

        for item in data.get("ASTypeAliases", []):
            by_namespace[namespace_for(item, default_namespace)].append(
                f"typedef {item['Target']} {item['Name']};"
            )

        for enum in data.get("ASEnums", []):
            values = [str(value) for value in enum.get("Values", [])]
            lines = [f"enum {enum['Name']} {{"]
            lines.extend(f"    {value}{',' if index + 1 < len(values) else ''}" for index, value in enumerate(values))
            lines.append("}")
            by_namespace[namespace_for(enum, default_namespace)].append("\n".join(lines))

        for as_type in data.get("ASTypes", []):
            lines = [f"class {as_type['Name']} {{"]
            lines.extend(f"    {member}" for member in type_members(as_type))
            lines.append("}")
            by_namespace[namespace_for(as_type, default_namespace)].append("\n".join(lines))

        for group_name in ("ASFunctions", "ASClassFunctions"):
            for function in data.get(group_name, []):
                by_namespace[namespace_for(function, default_namespace)].append(
                    declaration(
                        str(function["ReturnType"]),
                        str(function["Name"]),
                        str(function.get("Signature", "")),
                    )
                    + ";"
                )

        for constant in data.get("ASConstants", []):
            by_namespace[namespace_for(constant, default_namespace)].append(
                f"const {constant['Type']} {constant['Name']};"
            )

    return by_namespace


def render_predefined(declarations: dict[str, list[str]]) -> str:
    sections: list[str] = ["// Generated by tools/docs/as_predefiner/as_predefiner.py. Do not edit."]

    for namespace in sorted(declarations):
        body = "\n\n".join(declarations[namespace])
        if namespace:
            sections.append(f"namespace {namespace} {{\n{body}\n}}")
        else:
            sections.append(body)

    return "\n\n".join(sections) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--idl-dir", type=Path, default=DEFAULT_IDL_DIRECTORY,
                        help="Directory recursively containing *.idl.yaml files.")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT,
                        help="Path of the generated as.predefined file.")
    args = parser.parse_args()

    idl_files = list(args.idl_dir.rglob("*.idl.yaml"))
    if not idl_files:
        parser.error(f"no .idl.yaml files found under {args.idl_dir}")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_predefined(collect_declarations(idl_files)), encoding="utf-8")


if __name__ == "__main__":
    main()
