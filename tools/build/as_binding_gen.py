from __future__ import annotations

from collections.abc import Iterable

import generator
import idl


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


def _escape_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def _type_flags(flags: Iterable[str]) -> str:
    rendered = [generator.FLAG_MAP[flag] for flag in flags]
    return " | ".join(rendered) if rendered else "0"


def _operator_name(operator: str) -> str:
    return {
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
    }.get(operator, operator)


def _behaviour_declaration(
    as_type: idl.ASType,
    behaviour: idl.ASBehaviour,
) -> str:
    params = _registration_signature(behaviour.signature)

    match behaviour.type:
        case "Construct":
            return f"void f({params})" if params else "void f()"

        case "Destruct":
            return "void f()"

        case "Factory":
            return (
                f"{as_type.name}@ f({params})"
                if params
                else f"{as_type.name}@ f()"
            )

        case "AddRef" | "Release":
            return "void f()"

        case _:
            return f"void f({params})" if params else "void f()"


def generate_as_type_binding(
    as_type: idl.ASType,
    gen: generator.CodeWriter,
    restore_namespace: str = "",
) -> None:
    gen.push_as_namespace(as_type.namespace)

    gen.write(
        f'CE_REGISTER_TYPE("{as_type.name}", sizeof({as_type.cpp_type}), {_type_flags(as_type.flags)});'
    )

    for ctor in as_type.constructors:
        declaration = (
            f"void f({_registration_signature(ctor.signature)})"
            if ctor.signature
            else "void f()"
        )

        gen.write(
            f'CE_REGISTER_OBJECT_BEHAVIOUR("{as_type.name}", {generator.BEHAVIOUR_MAP["Construct"]}, "{declaration}", {ctor.cpp_function}, {generator.CALL_CONV_MAP["CDeclObjLast"]});'
        )

    for behaviour in as_type.behaviours:
        declaration = _behaviour_declaration(as_type, behaviour)

        gen.write(
            f'CE_REGISTER_OBJECT_BEHAVIOUR("{as_type.name}", {generator.BEHAVIOUR_MAP[behaviour.type]}, "{declaration}", {behaviour.cpp_function}, {generator.CALL_CONV_MAP[behaviour.calling_convention]});'
        )

    for method in as_type.methods:
        declaration = (
            f"{method.return_type} {method.name}({_registration_signature(method.signature)})"
        )

        if method.is_const:
            declaration += " const"

        gen.write(
            f'CE_REGISTER_OBJECT_METHOD("{as_type.name}", "{declaration}", {method.cpp_function}, {generator.CALL_CONV_MAP[method.calling_convention]});'
        )

    for operator in as_type.operators:
        declaration = (
            f"{operator.return_type} {_operator_name(operator.operator)}({_registration_signature(operator.signature)})"
        )
        if operator.is_const:
            declaration += " const"

        function_name = operator.generated_name or operator.cpp_function

        gen.write(
            f'CE_REGISTER_OBJECT_METHOD("{as_type.name}", "{declaration}", {function_name}, {generator.CALL_CONV_MAP[operator.calling_convention]});'
        )

    for prop in as_type.properties:
        gen.write(
            f'CE_REGISTER_OBJECT_PROPERTY("{as_type.name}", "{prop.type} {prop.as_member}", asOFFSET({as_type.cpp_type}, {prop.cpp_member}));'
        )

    gen.pop_as_namespace(restore_namespace)


def generate_as_function_binding(
    func: idl.ASFunction,
    gen: generator.CodeWriter,
    restore_namespace: str = "",
) -> None:
    declaration = (
        f"{func.return_type} {func.name}({_registration_signature(func.signature)})"
    )

    gen.push_as_namespace(func.namespace)

    gen.write(
        f'CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction("{declaration}", {func.cpp_function}, {generator.CALL_CONV_MAP[func.calling_convention]}));'
    )

    gen.pop_as_namespace(restore_namespace)


def generate_as_enum_binding(
    enum: idl.ASEnum,
    gen: generator.CodeWriter,
    restore_namespace: str = "",
) -> None:
    gen.push_as_namespace(enum.namespace)

    gen.write(
        f'CE_CHECK_AS(mScriptEngine.RegisterEnum("{enum.name}"));'
    )

    for value in enum.values:
        cpp_value = f"{enum.cpp_type}::{value}" if enum.cpp_type else value

        gen.write(
            f'CE_CHECK_AS(mScriptEngine.RegisterEnumValue("{enum.name}", "{value}", (int){cpp_value}));'
        )

    gen.pop_as_namespace(restore_namespace)


def generate_as_constant(
    constant: idl.ASConstant,
    gen: generator.CodeWriter,
    restore_namespace: str = "",
) -> None:
    gen.push_as_namespace(constant.namespace)

    if isinstance(constant.value, str):
        value = f'"{_escape_string(constant.value)}"'
    elif isinstance(constant.value, bool):
        value = "true" if constant.value else "false"
    else:
        value = str(constant.value)

    gen.write(
        f"static const {constant.type} s_{constant.name} = {value};"
    )

    gen.write(
        f'CE_CHECK_AS(mScriptEngine.RegisterGlobalProperty("const {constant.type} {constant.name}", (void*)&s_{constant.name}));'
    )

    gen.pop_as_namespace(restore_namespace)


def generate_as_alias(
    alias: idl.ASTypeAlias,
    gen: generator.CodeWriter,
    restore_namespace: str = "",
) -> None:
    gen.push_as_namespace(alias.namespace)

    gen.write(
        f'CE_CHECK_AS(mScriptEngine.RegisterTypedef("{alias.alias}", "{alias.type}"));'
    )

    gen.pop_as_namespace(restore_namespace)


def generate_as_declaration(
    declaration: idl.ASDeclaration,
    gen: generator.CodeWriter,
    restore_namespace: str = "",
) -> None:
    gen.push_as_namespace(declaration.namespace)

    gen.write(
        f"// AngelScript declaration: {declaration.name}"
    )

    gen.pop_as_namespace(restore_namespace)

def generate_as_class_function(
        func: idl.ASClassFunction,
        gen: generator.CodeWriter
) -> None:
    declaration = f"{func.return_type} {func.name}({_registration_signature(func.signature)})"
    gen.write(
        f'CE_REGISTER_GLOBAL({idl.CLASS_NAME}, this, "{declaration}", {func.name});'
    )
