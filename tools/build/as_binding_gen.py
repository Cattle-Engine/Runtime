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


def _type_flags(flags: Iterable[str], cpp_type: str) -> str:
    rendered = [
        f"asGetTypeTraits<{cpp_type}>()"
        if flag == "AutoGetFlags"
        else generator.FLAG_MAP[flag]
        for flag in flags
    ]
    return " | ".join(rendered) if rendered else "0"


def _generic_function_ref(function_name: str, callable: idl.ASBindableCallable) -> str:
    """Return the autowrapper expression for a free function."""
    # A generated helper has a unique, unambiguous symbol. For an application
    # function CppSignature selects the explicit-signature variant, which is
    # also how overloaded functions are disambiguated.
    if callable.cpp_signature and not callable.generated_name:
        return f"WRAP_FN_PR({function_name}, ({callable.cpp_signature}), {getattr(callable, 'cpp_return_type', '') or getattr(callable, 'return_type', 'void')})"
    return f"WRAP_FN({function_name})"


def _generic_object_ref(
    function_name: str,
    callable: idl.ASBindableCallable,
    calling_convention: str,
) -> str:
    wrapper = "WRAP_OBJ_FIRST" if calling_convention == "CDeclObjFirst" else "WRAP_OBJ_LAST"
    if callable.cpp_signature and not callable.generated_name:
        return f"{wrapper}_PR({function_name}, ({callable.cpp_signature}), {getattr(callable, 'cpp_return_type', '') or getattr(callable, 'return_type', 'void')})"
    return f"{wrapper}({function_name})"


def _generic_method_ref(as_type: idl.ASType, method: idl.ASMethod) -> str:
    if method.cpp_function.startswith("asMETHOD("):
        raise ValueError(
            f"Generic method '{method.name}' in '{as_type.name}' must use an unwrapped CppFunction name"
        )
    suffix = " const" if method.is_const else ""
    if method.cpp_signature:
        return (
            f"WRAP_MFN_PR({as_type.cpp_type}, {method.cpp_function}, "
            f"({method.cpp_signature}){suffix}, {method.cpp_return_type})"
        )
    return f"WRAP_MFN({as_type.cpp_type}, {method.cpp_function})"


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


def _behaviour_helper_name(
    as_type: idl.ASType,
    behaviour: idl.ASBehaviour,
    index: int,
) -> str:
    suffix = "" if index == 0 else f"_{index}"
    return _sanitize_symbol_part(f"{as_type.name}_{behaviour.type}_Generated{suffix}")


def _behaviour_cpp_parameter_type(as_type: idl.ASType, part: str) -> str:
    normalized = part.replace(" ", "")
    script_name = as_type.name

    if normalized == script_name:
        return as_type.cpp_type
    if normalized == f"const{script_name}&":
        return f"const {as_type.cpp_type}&"
    if normalized == f"{script_name}&":
        return f"{as_type.cpp_type}&"

    return _cpp_parameter_type(part)


def _behaviour_parameter_list(
    as_type: idl.ASType,
    behaviour: idl.ASBehaviour,
) -> str:
    signature = behaviour.cpp_signature or behaviour.signature
    params = ", ".join(
        f"{_behaviour_cpp_parameter_type(as_type, part)} arg{index}"
        for index, part in enumerate(_signature_parts(signature))
    )
    self_param = f"{as_type.cpp_type}* self"

    if behaviour.type == "Factory":
        return params

    if behaviour.type == "Construct":
        params = ", ".join(
            x for x in [
                params,
                self_param,
            ]
            if x
        )

    elif behaviour.type == "Destruct":
        params = self_param
    elif behaviour.calling_convention == "CDeclObjFirst":
        params = ", ".join(
            x for x in [
                self_param,
                params,
            ]
            if x
        )
    elif behaviour.calling_convention == "CDeclObjLast":
        params = ", ".join(
            x for x in [
                params,
                self_param,
            ]
            if x
        )

    return params


def _behaviour_return_type(behaviour: idl.ASBehaviour, as_type: idl.ASType) -> str:
    if behaviour.type == "Factory":
        return f"{as_type.cpp_type}*"
    return "void"


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
        f'CE_REGISTER_TYPE("{as_type.name}", sizeof({as_type.cpp_type}), {_type_flags(as_type.flags, as_type.cpp_type)});'
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
        function_name = behaviour.generated_name or behaviour.cpp_function
        if behaviour.calling_convention == "Generic":
            if behaviour.type in {"Destruct", "Destructor"} and not behaviour.generated_name:
                function_ref = f"WRAP_DES({as_type.cpp_type})"
            elif behaviour.type in {"Construct", "Constructor"} and not behaviour.generated_name:
                signature = behaviour.cpp_signature or behaviour.signature
                function_ref = f"WRAP_CON({as_type.cpp_type}, ({signature}))"
            elif behaviour.type == "Factory":
                function_ref = _generic_function_ref(function_name, behaviour)
            else:
                function_ref = _generic_object_ref(function_name, behaviour, "CDeclObjLast")
        else:
            function_ref = f"asFUNCTION({function_name})" if behaviour.generated_name else function_name

        gen.write(
            f'CE_REGISTER_OBJECT_BEHAVIOUR("{as_type.name}", {generator.BEHAVIOUR_MAP[behaviour.type]}, "{declaration}", {function_ref}, {generator.CALL_CONV_MAP[behaviour.calling_convention]});'
        )

    for method in as_type.methods:
        declaration = (
            f"{method.return_type} {method.name}({_registration_signature(method.signature)})"
        )

        if method.is_const:
            declaration += " const"

        function_name = method.generated_name or method.cpp_function
        if method.calling_convention == "Generic":
            function_ref = (
                _generic_object_ref(function_name, method, "CDeclObjFirst")
                if method.generated_name
                else _generic_method_ref(as_type, method)
            )
        else:
            function_ref = f"asFUNCTION({function_name})" if method.generated_name else function_name

        gen.write(
            f'CE_REGISTER_OBJECT_METHOD("{as_type.name}", "{declaration}", {function_ref}, {generator.CALL_CONV_MAP[method.calling_convention]});'
        )

    for operator in as_type.operators:
        declaration = (
            f"{operator.return_type} {_operator_name(operator.operator)}({_registration_signature(operator.signature)})"
        )
        if operator.is_const:
            declaration += " const"

        function_name = operator.generated_name or operator.cpp_function
        if operator.calling_convention == "Generic":
            function_ref = _generic_object_ref(
                function_name,
                operator,
                "CDeclObjFirst",
            )
        else:
            function_ref = f"asFUNCTION({function_name})" if operator.generated_name else function_name

        gen.write(
            f'CE_REGISTER_OBJECT_METHOD("{as_type.name}", "{declaration}", {function_ref}, {generator.CALL_CONV_MAP[operator.calling_convention]});'
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

    cpp_function = func.generated_name if func.inline_body and func.generated_name else func.cpp_function
    function_ref = (
        _generic_function_ref(cpp_function, func)
        if func.calling_convention == "Generic"
        else (f"asFUNCTION({cpp_function})" if func.inline_body and func.generated_name else cpp_function)
    )

    gen.write(
        f'CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction("{declaration}", {function_ref}, {generator.CALL_CONV_MAP[func.calling_convention]}));'
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

    cpp_type = constant.cpp_type or constant.type
    gen.write(f"static const {cpp_type} s_{constant.name} = {value};")

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
