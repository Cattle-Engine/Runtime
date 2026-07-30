# Takes a yaml file and generates a C++ binding for angelscript
import argparse
import yaml
import generator
from pathlib import Path
from enum import Enum
from typing import Any Final
from dataclasses import dataclass, field

CLASS_NAME: Final = generate_symbol_name() 

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
class ASBehaviour:
    type: str
    cpp_function: str
    signature: str = ""


@dataclass
class ASMethod:
    name: str
    return_type: str
    signature: str
    cpp_function: str
    is_const: bool = False


@dataclass
class ASOperator:
    operator: str
    return_type: str
    signature: str
    cpp_function: str


@dataclass
class ASType:
    name: str
    cpp_type: str
    properties: list[ASProperty] = field(default_factory=list)
    constructors: list[ASConstructor] = field(default_factory=list)
    behaviours: list[ASBehaviour] = field(default_factory=list)
    methods: list[ASMethod] = field(default_factory=list)
    operators: list[ASOperator] = field(default_factory=list)

@dataclass
class ASFunction:
    name: str
    return_type: str
    signature: str
    cpp_function: str
    namespace: str = ""


@dataclass
class ASEnum:
    name: str
    cpp_type: str
    values: list[str]


@dataclass
class ASConstant:
    name: str
    type: str
    value: Any


@dataclass
class ASBindingFile:
    required_cpp_headers: list[str]
    namespace: str
    types: list[ASType] = field(default_factory=list)
    functions: list[ASFunction] = field(default_factory=list)
    enums: list[ASEnum] = field(default_factory=list)
    constants: list[ASConstant] = field(default_factory=list)

class LogLevel(Enum):
    Debug = 1
    Info = 2
    Warn = 3
    Fatal = 4

def parse_as_property(data: dict[str, Any]) -> ASProperty:
    return ASProperty(
        type=data["Type"],
        cpp_member=data["CppMember"],
        as_member=data["ASMember"]
    )


def parse_as_constructor(data: dict[str, Any]) -> ASConstructor:
    return ASConstructor(
        signature=data.get("Signature", ""),
        cpp_function=data["CppFunction"]
    )


def parse_as_behaviour(data: dict[str, Any]) -> ASBehaviour:
    return ASBehaviour(
        type=data["Type"],
        cpp_function=data["CppFunction"],
        signature=data.get("Signature", "")
    )


def parse_as_method(data: dict[str, Any]) -> ASMethod:
    return ASMethod(
        name=data["Name"],
        return_type=data["ReturnType"],
        signature=data.get("Signature", ""),
        cpp_function=data["CppFunction"],
        is_const=data.get("IsConst", False)
    )


def parse_as_operator(data: dict[str, Any]) -> ASOperator:
    return ASOperator(
        operator=data["Operator"],
        return_type=data["ReturnType"],
        signature=data.get("Signature", ""),
        cpp_function=data["CppFunction"]
    )


def parse_as_type(data: dict[str, Any]) -> ASType:
    return ASType(
        name=data["Name"],
        cpp_type=data["CppType"],
        properties=[
            parse_as_property(x)
            for x in data.get("Properties", [])
        ],
        constructors=[
            parse_as_constructor(x)
            for x in data.get("Constructors", [])
        ],
        behaviours=[
            parse_as_behaviour(x)
            for x in data.get("Behaviours", [])
        ],
        methods=[
            parse_as_method(x)
            for x in data.get("Methods", [])
        ],
        operators=[
            parse_as_operator(x)
            for x in data.get("Operators", [])
        ]
    )

def parse_binding_file(data: dict[str, Any]) -> ASBindingFile:
    return ASBindingFile(
        required_cpp_headers=data.get("RequiredCppHeaders", []),
        namespace=data.get("ASNamespace", ""),

        types=[
            parse_as_type(x)
            for x in data.get("ASTypes", [])
        ],

        functions=[
            ASFunction(
                name=x["Name"],
                namespace=x.get("Namespace", data.get("ASNamespace", "")),
                return_type=x["ReturnType"],
                signature=x.get("Signature", ""),
                cpp_function=x["CppFunction"]
            )
            for x in data.get("ASFunctions", [])
        ],

        enums=[
            ASEnum(
                name=x["Name"],
                cpp_type=x.get("CppType", ""),
                values=x.get("Values", [])
            )
            for x in data.get("ASEnums", [])
        ],

        constants=[
            ASConstant(
                name=x["Name"],
                type=x["Type"],
                value=x["Value"]
            )
            for x in data.get("ASConstants", [])
        ]
    )

def validate_binding_file(binding: ASBindingFile) -> list[str]:
    errors: list[str] = []

    if not binding.namespace:
        errors.append("Missing ASNamespace")

    # Types
    type_names: set[str] = set()

    for as_type in binding.types:
        if not as_type.name:
            errors.append("ASType is missing Name")
            continue

        if as_type.name in type_names:
            errors.append(f"Duplicate ASType '{as_type.name}'")

        type_names.add(as_type.name)

        if not as_type.cpp_type:
            errors.append(
                f"ASType '{as_type.name}' is missing CppType"
            )

        property_names: set[str] = set()

        for prop in as_type.properties:
            if not prop.type:
                errors.append(
                    f"Property in '{as_type.name}' is missing Type"
                )

            if not prop.cpp_member:
                errors.append(
                    f"Property in '{as_type.name}' is missing CppMember"
                )

            if not prop.as_member:
                errors.append(
                    f"Property in '{as_type.name}' is missing ASMember"
                )

            if prop.as_member in property_names:
                errors.append(
                    f"Duplicate property '{prop.as_member}' in '{as_type.name}'"
                )

            property_names.add(prop.as_member)

        for constructor in as_type.constructors:
            if not constructor.cpp_function:
                errors.append(
                    f"Constructor in '{as_type.name}' missing CppFunction"
                )

        for behaviour in as_type.behaviours:
            if not behaviour.type:
                errors.append(
                    f"Behaviour in '{as_type.name}' missing Type"
                )

            if not behaviour.cpp_function:
                errors.append(
                    f"Behaviour '{behaviour.type}' in '{as_type.name}' missing CppFunction"
                )

        method_names: set[str] = set()

        for method in as_type.methods:
            if not method.name:
                errors.append(
                    f"Method in '{as_type.name}' missing Name"
                )

            if not method.return_type:
                errors.append(
                    f"Method '{method.name}' in '{as_type.name}' missing ReturnType"
                )

            if not method.cpp_function:
                errors.append(
                    f"Method '{method.name}' in '{as_type.name}' missing CppFunction"
                )

            if method.name in method_names:
                errors.append(
                    f"Duplicate method '{method.name}' in '{as_type.name}'"
                )

            method_names.add(method.name)

        for operator in as_type.operators:
            if not operator.operator:
                errors.append(
                    f"Operator in '{as_type.name}' missing Operator"
                )

            if not operator.return_type:
                errors.append(
                    f"Operator '{operator.operator}' in '{as_type.name}' missing ReturnType"
                )

            if not operator.cpp_function:
                errors.append(
                    f"Operator '{operator.operator}' in '{as_type.name}' missing CppFunction"
                )

    # Free functions
    function_names: set[str] = set()

    for function in binding.functions:
        if not function.name:
            errors.append("ASFunction missing Name")

        if not function.return_type:
            errors.append(
                f"Function '{function.name}' missing ReturnType"
            )

        if not function.cpp_function:
            errors.append(
                f"Function '{function.name}' missing CppFunction"
            )

        if function.name in function_names:
            errors.append(
                f"Duplicate ASFunction '{function.name}'"
            )

        function_names.add(function.name)

    # Enums
    enum_names: set[str] = set()

    for enum in binding.enums:
        if not enum.name:
            errors.append("ASEnum missing Name")

        if not enum.values:
            errors.append(
                f"Enum '{enum.name}' has no values"
            )

        if enum.name in enum_names:
            errors.append(
                f"Duplicate ASEnum '{enum.name}'"
            )

        enum_names.add(enum.name)

    # Constants
    constant_names: set[str] = set()

    for constant in binding.constants:
        if not constant.name:
            errors.append("ASConstant missing Name")

        if not constant.type:
            errors.append(
                f"Constant '{constant.name}' missing Type"
            )

        if constant.name in constant_names:
            errors.append(
                f"Duplicate ASConstant '{constant.name}'"
            )

        constant_names.add(constant.name)

    return errors

def log(level: LogLevel, message: str, *args) -> None:
    message = message.format(*args)
    print(f"[{level.name}] {message}")

def generate_comment(comment: str) -> str:
    return f"/* {comment} */"

def generate_header_list(headers: list[str]) -> str:
    return "\n".join(f'#include "{header}"' for header in headers)

def generate_cpp_source(binding: ASBindingFile) -> str:
    return ""

def generate_cpp_header(binding: ASBindingFile) -> str:
    out: str
    
    
    return ""

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser("Build helper for CE to turn a yaml registration file into AngelScript bindings")
    parser.add_argument("--source_dir", type=Path, help="Directory for source")
    parser.add_argument("--include_dir", type=Path, help="Directory for include")
    parser.add_argument("--yaml_file", type=Path, help="Path to the yaml file")
    parser.add_argument("--output_header", type=Path, help="Output for the generated header")
    parser.add_argument("--output_source", type=Path, help="Output for the generated source")
    return parser.parse_args()

def main() -> int:
    try:
        args = parse_args()

        with open(args.yaml_file) as f:
            yaml_data = yaml.safe_load(f)

        bindings = parse_binding_file(yaml_data)
        errors = validate_binding_file(bindings)

        if errors:
            for error in errors:
                log(LogLevel.Fatal, error)

            return 1
        
        includes: str = generate_header_list(bindings.required_cpp_headers) # Combined with generated_source
        generated_source: str = generate_cpp_source(bindings)

        source: str = (
            f"{generate_comment('AUTO GENERATED BY CE IDL GENERATOR, DO NOT MODIFY')}\n"
            f"{includes}\n"
            f"{generated_source}"
        )

        args.output_source.parent.mkdir(parents=True, exist_ok=True)
        args.output_source.write_text(source, encoding="utf-8")

        generated_header: str = generate_cpp_header(bindings)

        header: str = (
            f"{generate_comment('AUTO GENERATED BY CE IDL GENERATOR, DO NOT MODIFY')}\n"
            f"{generated_header}"
        )

        args.output_header.parent.mkdir(parents=True, exist_ok=True)
        args.output_header.write_text(header, encoding="utf-8")

    except OSError:
        log(LogLevel.Fatal, "Yaml file was not found!")
        return 1
    

    return 0

if __name__ == "__main__":
    raise SystemExit(main())