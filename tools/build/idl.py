# Takes a yaml file and generates a C++ binding for angelscript
from __future__ import annotations

import argparse
import os
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Any, Final

import yaml

import as_binding_gen
import generator


CLASS_NAME: Final = generator.generate_symbol_name()


@dataclass
class ASProperty:
    type: str
    cpp_member: str
    as_member: str


@dataclass
class ASConstructor:
    signature: str
    cpp_function: str


@dataclass
class ASBindableCallable:
    cpp_function: str = ""
    inline_body: str = ""
    generated_name: str = ""
    calling_convention: str = ""


@dataclass
class ASBehaviour(ASBindableCallable):
    type: str = ""
    cpp_function: str = ""
    signature: str = ""
    calling_convention: str = "CDecl"


@dataclass
class ASMethod(ASBindableCallable):
    name: str = ""
    return_type: str = ""
    signature: str = ""
    is_const: bool = False
    calling_convention: str = "ThisCall"


@dataclass
class ASOperator(ASBindableCallable):
    operator: str = ""
    return_type: str = ""
    signature: str = ""
    calling_convention: str = "CDeclObjFirst"
    is_const: bool = True

@dataclass
class ASType:
    name: str
    cpp_type: str
    namespace: str = ""
    flags: list[str] = field(default_factory=list)
    properties: list[ASProperty] = field(default_factory=list)
    constructors: list[ASConstructor] = field(default_factory=list)
    behaviours: list[ASBehaviour] = field(default_factory=list)
    methods: list[ASMethod] = field(default_factory=list)
    operators: list[ASOperator] = field(default_factory=list)


@dataclass
class ASFunction(ASBindableCallable):
    name: str = ""
    return_type: str = ""
    signature: str = ""
    namespace: str = ""
    calling_convention: str = "CDecl"


@dataclass
class ASEnum:
    name: str
    cpp_type: str
    values: list[str]
    namespace: str = ""


@dataclass
class ASConstant:
    name: str
    type: str
    value: Any
    namespace: str = ""


@dataclass
class ASTypeAlias:
    type: str
    alias: str
    namespace: str = ""


@dataclass
class ASDeclaration:
    name: str
    namespace: str = ""


@dataclass
class ASClassFunction(ASBindableCallable):
    name: str = ""
    return_type: str = ""
    signature: str = ""
    cpp_signature: str = "" # used for the actual function
    cpp_class: str = ""
    cpp_method: str = ""
    as_namespace: str = ""
    calling_convention: str = "ThisCallAsGlobal"


@dataclass
class ASBindingFile:
    required_cpp_headers: list[str]
    namespace: str
    types: list[ASType] = field(default_factory=list)
    functions: list[ASFunction] = field(default_factory=list)
    enums: list[ASEnum] = field(default_factory=list)
    constants: list[ASConstant] = field(default_factory=list)
    aliases: list[ASTypeAlias] = field(default_factory=list)
    declarations: list[ASDeclaration] = field(default_factory=list)
    class_functions: list[ASClassFunction] = field(default_factory=list)


class LogLevel(Enum):
    Debug = 1
    Info = 2
    Warn = 3
    Fatal = 4


def _maybe_get(data: dict[str, Any], *keys: str, default: Any = None) -> Any:
    for key in keys:
        if key in data:
            return data[key]
    return default


def parse_as_property(data: dict[str, Any]) -> ASProperty:
    return ASProperty(
        type=data["Type"],
        cpp_member=data["CppMember"],
        as_member=data["ASMember"],
    )


def parse_as_constructor(data: dict[str, Any]) -> ASConstructor:
    return ASConstructor(
        signature=data.get("Signature", ""),
        cpp_function=data["CppFunction"],
    )


def parse_as_behaviour(data: dict[str, Any]) -> ASBehaviour:
    return ASBehaviour(
        type=data["Type"],
        cpp_function=data.get("CppFunction", ""),
        signature=data.get("Signature", ""),
        inline_body=data.get("Body", ""),
        calling_convention=data.get("CallingConvention", "CDecl"),
    )

def parse_as_method(data: dict[str, Any]) -> ASMethod:
    return ASMethod(
        name=data["Name"],
        return_type=data["ReturnType"],
        signature=data.get("Signature", ""),
        cpp_function=data.get("CppFunction", ""),
        inline_body=data.get("Body", ""),
        is_const=data.get("IsConst", False),
        calling_convention=data.get("CallingConvention", "ThisCall"),
    )


def parse_as_operator(data: dict[str, Any]) -> ASOperator:
    return ASOperator(
        operator=data["Operator"],
        return_type=data["ReturnType"],
        signature=data.get("Signature", ""),
        cpp_function=data.get("CppFunction", ""),
        inline_body=data.get("Body", ""),
        calling_convention=data.get("CallingConvention", "CDeclObjFirst"),
        is_const=data.get("IsConst", True),
    )


def parse_as_type(data: dict[str, Any], default_namespace: str = "") -> ASType:
    return ASType(
        name=data["Name"],
        cpp_type=data["CppType"],
        namespace=data.get("Namespace", default_namespace),
        flags=list(_maybe_get(data, "Flags", "ASFlags", default=[])),
        properties=[parse_as_property(x) for x in data.get("Properties", [])],
        constructors=[parse_as_constructor(x) for x in data.get("Constructors", [])],
        behaviours=[parse_as_behaviour(x) for x in data.get("Behaviours", [])],
        methods=[parse_as_method(x) for x in data.get("Methods", [])],
        operators=[parse_as_operator(x) for x in data.get("Operators", [])],
    )


def parse_binding_file(data: dict[str, Any]) -> ASBindingFile:
    default_namespace = data.get("ASNamespace", "")

    return ASBindingFile(
        required_cpp_headers=data.get("RequiredCppHeaders", []),
        namespace=default_namespace,
        types=[parse_as_type(x, default_namespace) for x in data.get("ASTypes", [])],
        functions=[
            ASFunction(
                name=x["Name"],
                namespace=x.get("Namespace", default_namespace),
                return_type=x["ReturnType"],
                signature=x.get("Signature", ""),
                cpp_function=x.get("CppFunction", ""),
                inline_body=x.get("Body", ""),
                generated_name=x.get("GeneratedName", ""),
                calling_convention=x.get("CallingConvention", "CDecl"),
            )
            for x in data.get("ASFunctions", [])
        ],
        enums=[
            ASEnum(
                name=x["Name"],
                cpp_type=x.get("CppType", ""),
                values=x.get("Values", []),
                namespace=x.get("Namespace", default_namespace),
            )
            for x in data.get("ASEnums", [])
        ],
        constants=[
            ASConstant(
                name=x["Name"],
                type=x["Type"],
                value=x["Value"],
                namespace=x.get("Namespace", default_namespace),
            )
            for x in data.get("ASConstants", [])
        ],
        aliases=[
            ASTypeAlias(
                type=x["Target"],
                alias=x["Name"],
                namespace=x.get("Namespace", default_namespace),
            )
            for x in data.get("ASTypeAliases", [])
        ],
        declarations=[
            ASDeclaration(
                name=x["Name"],
                namespace=x.get("Namespace", default_namespace),
            )
            for x in data.get("ASDeclarations", [])
        ],
        class_functions=[
            ASClassFunction(
                name=x["Name"],
                return_type=x["ReturnType"],
                signature=x.get("Signature", ""),
                cpp_class=x.get("Class", ""),
                cpp_method=x.get("CppMethod", ""),
                inline_body=x.get("Function", x.get("Body", "")),
                generated_name=x.get("GeneratedName", ""),
                cpp_signature=x.get("CppSignature", ""),
                as_namespace=x.get("Namespace", default_namespace),
                calling_convention=x.get(
                    "CallingConvention",
                    "ThisCallAsGlobal"
                ),
            )
            for x in data.get("ASClassFunctions", [])
        ],
    )


def validate_binding_file(binding: ASBindingFile) -> list[str]:
    errors: list[str] = []

    if not binding.namespace:
        errors.append("Missing ASNamespace")

    type_names: set[str] = set()
    for as_type in binding.types:
        if not as_type.name:
            errors.append("ASType is missing Name")
            continue

        if as_type.name in type_names:
            errors.append(f"Duplicate ASType '{as_type.name}'")
        type_names.add(as_type.name)

        if not as_type.cpp_type:
            errors.append(f"ASType '{as_type.name}' is missing CppType")

        if not as_type.flags:
            errors.append(f"ASType '{as_type.name}' is missing Flags")
        else:
            for flag in as_type.flags:
                if flag not in generator.FLAG_MAP:
                    errors.append(f"ASType '{as_type.name}' has unknown flag '{flag}'")

        property_names: set[str] = set()
        for prop in as_type.properties:
            if not prop.type:
                errors.append(f"Property in '{as_type.name}' is missing Type")
            if not prop.cpp_member:
                errors.append(f"Property in '{as_type.name}' is missing CppMember")
            if not prop.as_member:
                errors.append(f"Property in '{as_type.name}' is missing ASMember")
            if prop.as_member in property_names:
                errors.append(
                    f"Duplicate property '{prop.as_member}' in '{as_type.name}'"
                )
            property_names.add(prop.as_member)

        for constructor in as_type.constructors:
            if not constructor.cpp_function:
                errors.append(f"Constructor in '{as_type.name}' missing CppFunction")

        for behaviour in as_type.behaviours:
            if not behaviour.type:
                errors.append(f"Behaviour in '{as_type.name}' missing Type")
            elif behaviour.type not in generator.BEHAVIOUR_MAP:
                errors.append(
                    f"Behaviour '{behaviour.type}' in '{as_type.name}' is not supported"
                )
            if not behaviour.cpp_function and not behaviour.inline_body:
                errors.append(
                    f"Behaviour '{behaviour.type}' in '{as_type.name}' missing CppFunction or Body"
                )
            if behaviour.calling_convention not in generator.CALL_CONV_MAP:
                errors.append(
                    f"Behaviour '{behaviour.type}' in '{as_type.name}' has unknown calling convention '{behaviour.calling_convention}'"
                )

        method_names: set[str] = set()
        for method in as_type.methods:
            if not method.name:
                errors.append(f"Method in '{as_type.name}' missing Name")
            if not method.return_type:
                errors.append(
                    f"Method '{method.name}' in '{as_type.name}' missing ReturnType"
                )
            if not method.cpp_function and not method.inline_body:
                errors.append(
                    f"Method '{method.name}' in '{as_type.name}' missing CppFunction or Body"
                )
            if method.calling_convention not in generator.CALL_CONV_MAP:
                errors.append(
                    f"Method '{method.name}' in '{as_type.name}' has unknown calling convention '{method.calling_convention}'"
                )
            if method.name in method_names:
                errors.append(f"Duplicate method '{method.name}' in '{as_type.name}'")
            method_names.add(method.name)

        for operator in as_type.operators:
            if not operator.operator:
                errors.append(f"Operator in '{as_type.name}' missing Operator")
            if not operator.return_type:
                errors.append(
                    f"Operator '{operator.operator}' in '{as_type.name}' missing ReturnType"
                )
            if not operator.cpp_function and not operator.inline_body:
                errors.append(
                    f"Operator '{operator.operator}' in '{as_type.name}' missing CppFunction or Body"
                )
            if operator.calling_convention not in {"CDeclObjFirst", "CDeclObjLast"}:
                errors.append(
                    f"Operator '{operator.operator}' in '{as_type.name}' has unsupported calling convention '{operator.calling_convention}'"
                )

    function_names: set[str] = set()
    for function in binding.functions:
        if not function.name:
            errors.append("ASFunction missing Name")
        if not function.return_type:
            errors.append(f"Function '{function.name}' missing ReturnType")
        if not function.cpp_function and not function.inline_body:
            errors.append(f"Function '{function.name}' missing CppFunction or Body")
        if function.calling_convention not in generator.CALL_CONV_MAP:
            errors.append(
                f"Function '{function.name}' has unknown calling convention '{function.calling_convention}'"
            )
        if function.name in function_names:
            errors.append(f"Duplicate ASFunction '{function.name}'")
        function_names.add(function.name)

    enum_names: set[str] = set()
    for enum in binding.enums:
        if not enum.name:
            errors.append("ASEnum missing Name")
        if not enum.values:
            errors.append(f"Enum '{enum.name}' has no values")
        if enum.name in enum_names:
            errors.append(f"Duplicate ASEnum '{enum.name}'")
        enum_names.add(enum.name)

    constant_names: set[str] = set()
    for constant in binding.constants:
        if not constant.name:
            errors.append("ASConstant missing Name")
        if not constant.type:
            errors.append(f"Constant '{constant.name}' missing Type")
        if constant.name in constant_names:
            errors.append(f"Duplicate ASConstant '{constant.name}'")
        constant_names.add(constant.name)

    alias_names: set[str] = set()
    for alias in binding.aliases:
        if not alias.alias:
            errors.append("ASTypeAlias missing Name")
        if not alias.type:
            errors.append(f"ASTypeAlias '{alias.alias}' missing Target")
        if alias.alias in alias_names:
            errors.append(f"Duplicate ASTypeAlias '{alias.alias}'")
        alias_names.add(alias.alias)

    declaration_names: set[str] = set()
    for declaration in binding.declarations:
        if not declaration.name:
            errors.append("ASDeclaration missing Name")
        if declaration.name in declaration_names:
            errors.append(f"Duplicate ASDeclaration '{declaration.name}'")
        declaration_names.add(declaration.name)

    class_function_names: set[str] = set()
    for class_function in binding.class_functions:
        if not class_function.name:
            errors.append("ASClassFunction missing Name")
        if not class_function.return_type:
            errors.append(f"ASClassFunction '{class_function.name}' missing ReturnType")
        if not class_function.inline_body:
            if not class_function.cpp_class:
                errors.append(f"ASClassFunction '{class_function.name}' missing Class")
            if not class_function.cpp_method:
                errors.append(f"ASClassFunction '{class_function.name}' missing CppMethod")
        if class_function.calling_convention not in generator.CALL_CONV_MAP:
            errors.append(
                f"ASClassFunction '{class_function.name}' has unknown calling convention '{class_function.calling_convention}'"
            )
        if class_function.name in class_function_names:
            errors.append(f"Duplicate ASClassFunction '{class_function.name}'")
        class_function_names.add(class_function.name)

    return errors


def log(level: LogLevel, message: str, *args: object) -> None:
    message = message.format(*args)
    print(f"[{level.name}] {message}")


def _signature_parts(signature: str) -> list[str]:
    signature = signature.strip()
    if not signature:
        return []

    parts: list[str] = []
    start = 0
    angle_depth = 0
    paren_depth = 0
    bracket_depth = 0

    for index, char in enumerate(signature):
        if char == "<":
            angle_depth += 1
        elif char == ">":
            angle_depth = max(0, angle_depth - 1)
        elif char == "(":
            paren_depth += 1
        elif char == ")":
            paren_depth = max(0, paren_depth - 1)
        elif char == "[":
            bracket_depth += 1
        elif char == "]":
            bracket_depth = max(0, bracket_depth - 1)
        elif (
            char == ","
            and angle_depth == 0
            and paren_depth == 0
            and bracket_depth == 0
        ):
            part = signature[start:index].strip()
            if part:
                parts.append(part)
            start = index + 1

    tail = signature[start:].strip()
    if tail:
        parts.append(tail)

    return parts


def _registration_signature(signature: str) -> str:
    return ", ".join(_signature_parts(signature))


def _cpp_parameter_type(part: str) -> str:
    tokens = part.split()
    if len(tokens) <= 1:
        return part

    tail = tokens[-1]
    if tail in {
        "const",
        "volatile",
        "signed",
        "unsigned",
        "short",
        "long",
        "int",
        "float",
        "double",
        "bool",
        "char",
        "void",
    }:
        return part
    if tail.endswith("&") or tail.endswith("*"):
        return part
    return " ".join(tokens[:-1])


def _cpp_parameter_list(signature: str) -> str:
    parts = _signature_parts(signature)
    return ", ".join(
        f"{_cpp_parameter_type(part)} arg{index}"
        for index, part in enumerate(parts)
    )


def _argument_list(signature: str) -> str:
    parts = _signature_parts(signature)
    return ", ".join(f"arg{index}" for index, _ in enumerate(parts))


def _header_include_path(output_header: Path, output_source: Path) -> str:
    include_path = os.path.relpath(output_header, start=output_source.parent)
    return Path(include_path).as_posix()


def _format_cpp_include(include: str) -> str:
    stripped = include.strip()
    if stripped.startswith("<") and stripped.endswith(">"):
        return stripped
    if stripped.startswith('"') and stripped.endswith('"'):
        return stripped
    return f'"{stripped}"'


def _sanitize_symbol_part(value: str) -> str:
    sanitized: list[str] = []
    last_was_underscore = False

    for char in value:
        if char.isalnum():
            sanitized.append(char)
            last_was_underscore = False
        elif not last_was_underscore:
            sanitized.append("_")
            last_was_underscore = True

    result = "".join(sanitized).strip("_")
    return result or "Symbol"


def _operator_helper_name(as_type: ASType, operator: ASOperator, index: int) -> str:
    return _sanitize_symbol_part(f"{as_type.name}_{operator.operator}_{index}")


def _operator_self_parameter(as_type: ASType, operator: ASOperator) -> str:
    if operator.is_const:
        return f"const {as_type.cpp_type}& self"
    return f"{as_type.cpp_type}& self"


def _operator_cpp_parameter_type(as_type: ASType, part: str) -> str:
    normalized = part.replace(" ", "")
    script_name = as_type.name

    if normalized == script_name:
        return as_type.cpp_type
    if normalized == f"const{script_name}&":
        return f"const {as_type.cpp_type}&"
    if normalized == f"{script_name}&":
        return f"{as_type.cpp_type}&"

    return _cpp_parameter_type(part)


def _operator_parameter_list(as_type: ASType, operator: ASOperator) -> str:
    self_param = _operator_self_parameter(as_type, operator)
    parts = [
        _operator_cpp_parameter_type(as_type, part)
        for part in _signature_parts(operator.signature)
    ]
    params = ", ".join(f"{part} arg{index}" for index, part in enumerate(parts))

    if operator.calling_convention == "CDeclObjLast":
        return ", ".join([part for part in [params, self_param] if part])

    return ", ".join([part for part in [self_param, params] if part])


def generate_cpp_source(binding: ASBindingFile, header_include: str) -> str:
    gen = generator.CodeWriter()
    inline_operator_helpers: list[tuple[str, ASType, ASOperator]] = []
    inline_behaviour_helpers: list[tuple[str, ASType, ASBehaviour]] = []

    for as_type in binding.types:
        behaviour_name_counts: dict[str, int] = {}

        for index, operator in enumerate(as_type.operators):
            if operator.inline_body:
                operator.generated_name = _operator_helper_name(as_type, operator, index)
                inline_operator_helpers.append((operator.generated_name, as_type, operator))

        for behaviour in as_type.behaviours:
            if behaviour.inline_body:
                inline_index = behaviour_name_counts.get(behaviour.type, 0)
                behaviour_name_counts[behaviour.type] = inline_index + 1
                behaviour.generated_name = as_binding_gen._behaviour_helper_name(
                    as_type,
                    behaviour,
                    inline_index,
                )
                inline_behaviour_helpers.append(
                    (behaviour.generated_name, as_type, behaviour)
                )

    gen.write(generator.generate_comment("GENERATED BY THE CE IDL, DO NOT MODIFY"))
    gen.write("#include <angelscript.h>")
    gen.write(f'#include "{header_include}"')

    for include in binding.required_cpp_headers:
        gen.write(f"#include {_format_cpp_include(include)}")

    gen.write("")
    if inline_operator_helpers or inline_behaviour_helpers:
        gen.begin_block("namespace")

        for helper_name, as_type, operator in inline_operator_helpers:
            declaration = (
                f"static {operator.return_type} {helper_name}"
                f"({_operator_parameter_list(as_type, operator)})"
            )
            gen.begin_function(declaration)
            for line in operator.inline_body.splitlines():
                gen.write(line)
            gen.end_block()
            gen.write("")

        for helper_name, as_type, behaviour in inline_behaviour_helpers:
            declaration = (
                f"static {as_binding_gen._behaviour_return_type(behaviour, as_type)} {helper_name}"
                f"({as_binding_gen._behaviour_parameter_list(as_type, behaviour)})"
            )
            gen.begin_function(declaration)
            for line in behaviour.inline_body.splitlines():
                gen.write(line)
            gen.end_block()
            gen.write("")

        gen.end_block()
        gen.write("")

    gen.begin_namespace("CE::Scripting::Bindings")

    for class_function in binding.class_functions:
        sig = class_function.cpp_signature or class_function.signature
        params = _cpp_parameter_list(sig)

        declaration = (
            f"{class_function.return_type} "
            f"{CLASS_NAME}::{class_function.name}({params})"
        )

        gen.begin_function(declaration)

        if class_function.inline_body:
            for line in class_function.inline_body.splitlines():
                gen.write(line)
        else:
            call_args = _argument_list(sig)

            call = (
                f"mRuntime.{class_function.cpp_class}."
                f"{class_function.cpp_method}({call_args})"
                if call_args
                else
                f"mRuntime.{class_function.cpp_class}."
                f"{class_function.cpp_method}()"
            )

            if class_function.return_type == "void":
                gen.write(f"{call};")
            else:
                gen.write(f"return {call};")

        gen.end_block()
        gen.write("")

    gen.begin_function(f"bool {CLASS_NAME}::RegisterBindings()")
    gen.write(f'mScriptEngine.SetDefaultNamespace("{binding.namespace}");')

    for as_type in binding.types:
        as_binding_gen.generate_as_type_binding(as_type, gen, binding.namespace)

    for as_function in binding.functions:
        as_binding_gen.generate_as_function_binding(as_function, gen, binding.namespace)

    for as_enum in binding.enums:
        as_binding_gen.generate_as_enum_binding(as_enum, gen, binding.namespace)

    for as_constant in binding.constants:
        as_binding_gen.generate_as_constant(as_constant, gen, binding.namespace)

    for alias in binding.aliases:
        as_binding_gen.generate_as_alias(alias, gen, binding.namespace)

    for declaration in binding.declarations:
        as_binding_gen.generate_as_declaration(declaration, gen, binding.namespace)

    for class_function in binding.class_functions:
        declaration = f"{class_function.return_type} {class_function.name}({_registration_signature(class_function.signature)})"
        gen.write(f'mScriptEngine.SetDefaultNamespace("{class_function.as_namespace}");')
        gen.write(
            f'CE_REGISTER_GLOBAL({CLASS_NAME}, this, "{declaration}", {class_function.name});'
        )

    gen.write('mScriptEngine.SetDefaultNamespace("");')
    gen.write("return true;")
    gen.end_block()
    gen.end_block()

    return gen.build()


def generate_cpp_header(binding: ASBindingFile) -> str:
    gen = generator.CodeWriter()

    gen.write(generator.generate_comment("GENERATED BY THE CE IDL, DO NOT MODIFY"))
    gen.write("#pragma once")

    for include in binding.required_cpp_headers:
        gen.write(f"#include {_format_cpp_include(include)}")

    gen.write(f'#include "{generator.ANGELSCRIPT_CLASS_INCLUDE}"')
    gen.write(f'#include "{generator.ANGELSCRIPT_MACRO_INCUDE}"')
    gen.write("")
    gen.begin_namespace("CE::Scripting::Bindings")
    gen.begin_class(CLASS_NAME, "public IScriptBinding")
    gen.write("public:")
    gen.indent()
    gen.write("bool RegisterBindings() override;")
    gen.dedent()

    if binding.class_functions:
        gen.write("")
        gen.write("private:")
        gen.indent()
        for class_function in binding.class_functions:
            sig = class_function.cpp_signature or class_function.signature
            params = _cpp_parameter_list(sig)
            gen.write(f"{class_function.return_type} {class_function.name}({params});")
        gen.dedent()

    gen.end_class()
    gen.end_block()

    return gen.build()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        "Build helper for CE to turn a yaml registration file into AngelScript bindings"
    )
    parser.add_argument("--source_dir", type=Path, required=False, help="Directory for source")
    parser.add_argument("--include_dir", type=Path, required=False, help="Directory for include")
    parser.add_argument("--yaml_file", type=Path, required=True, help="Path to the yaml file")
    parser.add_argument("--output_header", type=Path, required=True, help="Output for the generated header")
    parser.add_argument("--output_source", type=Path, required=True, help="Output for the generated source")
    return parser.parse_args()


def main() -> int:
    try:
        args = parse_args()

        with args.yaml_file.open(encoding="utf-8") as handle:
            yaml_data = yaml.safe_load(handle) or {}

        bindings = parse_binding_file(yaml_data)
        errors = validate_binding_file(bindings)

        if errors:
            for error in errors:
                log(LogLevel.Fatal, error)
            return 1

        header_include = _header_include_path(args.output_header, args.output_source)
        generated_source = generate_cpp_source(bindings, header_include)
        generated_header = generate_cpp_header(bindings)

        metadata = args.output_header.with_suffix(".json")
        metadata.write_text(
            f'{{"class_name": "{CLASS_NAME}"}}',
            encoding="utf-8",
        )

        args.output_source.parent.mkdir(parents=True, exist_ok=True)
        args.output_source.write_text(generated_source, encoding="utf-8")

        args.output_header.parent.mkdir(parents=True, exist_ok=True)
        args.output_header.write_text(generated_header, encoding="utf-8")

    except OSError:
        log(LogLevel.Fatal, "Yaml file was not found!")
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
