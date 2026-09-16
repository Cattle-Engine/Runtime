import argparse
from dataclasses import dataclass, field
from pathlib import Path
from collections import defaultdict
from typing import Any

import markdown_generator
import yaml


@dataclass
class DocumentationInfo:
    name: str
    namespace: str = ""
    description: str = ""
    return_type: str = ""
    signature: str = ""


@dataclass
class TypeDocumentation:
    name: str
    cpp_type: str = ""
    namespace: str = ""
    flags: list[str] = field(default_factory=list)
    description: str = ""
    properties: list[dict[str, Any]] = field(default_factory=list)
    constructors: list[dict[str, Any]] = field(default_factory=list)
    behaviours: list[dict[str, Any]] = field(default_factory=list)
    methods: list[dict[str, Any]] = field(default_factory=list)
    operators: list[dict[str, Any]] = field(default_factory=list)


@dataclass
class EnumDocumentation:
    name: str
    cpp_type: str = ""
    namespace: str = ""
    values: list[Any] = field(default_factory=list)


@dataclass
class BindingDocumentation:
    namespace: str = ""
    global_description: str = ""
    types: list[TypeDocumentation] = field(default_factory=list)
    functions: list[DocumentationInfo] = field(default_factory=list)
    enums: list[EnumDocumentation] = field(default_factory=list)
    constants: list[dict[str, Any]] = field(default_factory=list)
    aliases: list[dict[str, Any]] = field(default_factory=list)
    declarations: list[dict[str, Any]] = field(default_factory=list)
    class_functions: list[DocumentationInfo] = field(default_factory=list)


def _description(data: dict[str, Any]) -> str:
    return str(data.get("Description", "")).strip() or "No description given"


def _namespace(data: dict[str, Any], default: str) -> str:
    return data.get("Namespace") or default


def parse_binding_file(binding_file: Path) -> BindingDocumentation:
    with binding_file.open(encoding="utf-8") as handle:
        data = yaml.safe_load(handle) or {}

    default_namespace = data.get("ASNamespace", "")

    return BindingDocumentation(
        namespace=default_namespace,
        global_description=data.get("Documentation", ""),
        types=[
            TypeDocumentation(
                name=x["Name"],
                cpp_type=x.get("CppType", ""),
                namespace=_namespace(x, default_namespace),
                flags=x.get("Flags", x.get("ASFlags", [])),
                description=_description(x),
                properties=x.get("Properties", []),
                constructors=x.get("Constructors", []),
                behaviours=x.get("Behaviours", []),
                methods=x.get("Methods", []),
                operators=x.get("Operators", []),
            )
            for x in data.get("ASTypes", [])
        ],

        functions=[
            DocumentationInfo(
                name=x["Name"],
                namespace=_namespace(x, default_namespace),
                return_type=x.get("ReturnType", ""),
                signature=x.get("Signature", ""),
                description=_description(x),
            )
            for x in data.get("ASFunctions", [])
        ],

        enums=[
            EnumDocumentation(
                name=x["Name"],
                cpp_type=x.get("CppType", ""),
                namespace=_namespace(x, default_namespace),
                values=x.get("Values", []),
            )
            for x in data.get("ASEnums", [])
        ],

        constants=data.get("ASConstants", []),
        aliases=data.get("ASTypeAliases", []),
        declarations=data.get("ASDeclarations", []),

        class_functions=[
            DocumentationInfo(
                name=x["Name"],
                namespace=_namespace(x, default_namespace),
                return_type=x.get("ReturnType", ""),
                signature=x.get("Signature", ""),
                description=_description(x),
            )
            for x in data.get("ASClassFunctions", [])
        ],
    )


def write_description(
    generator: markdown_generator.MarkdownWriter,
    description: str,
) -> None:
    generator.write_text(description.strip() or "No description given")
    generator.newline()


def write_signature(
    generator: markdown_generator.MarkdownWriter,
    signature: str,
) -> None:
    if not signature:
        return

    generator.write_text("Signature:")
    generator.write_single_line_codeblock(signature, "angelscript")
    generator.newline()


def generate_property_docs(
    generator: markdown_generator.MarkdownWriter,
    properties: list[dict[str, Any]],
) -> None:
    if not properties:
        return

    generator.write_header("Properties", 3)

    for prop in properties:
        name = prop.get("ASMember", "Unknown")
        prop_type = prop.get("Type", "")

        generator.write_header(name, 4)
        generator.write_text(f"Type: `{prop_type}`")

        if prop.get("Description"):
            write_description(generator, prop["Description"])
        else:
            generator.newline()


def generate_constructor_docs(
    generator: markdown_generator.MarkdownWriter,
    constructors: list[dict[str, Any]],
) -> None:
    if not constructors:
        return

    generator.write_header("Constructors", 3)

    for constructor in constructors:
        signature = constructor.get("Signature", "")
        cpp_function = constructor.get("CppFunction", "")

        generator.write_header(
            signature or cpp_function or "Constructor",
            4,
        )

        if signature:
            write_signature(generator, signature)

        generator.newline()


def generate_behaviour_docs(
    generator: markdown_generator.MarkdownWriter,
    behaviours: list[dict[str, Any]],
) -> None:
    if not behaviours:
        return

    generator.write_header("Behaviours", 3)

    for behaviour in behaviours:
        behaviour_type = behaviour.get("Type", "Unknown")
        signature = behaviour.get("Signature", "")

        generator.write_header(behaviour_type, 4)

        if signature:
            write_signature(generator, signature)

        if behaviour.get("Description"):
            write_description(generator, behaviour["Description"])
        else:
            generator.newline()


def generate_method_docs(
    generator: markdown_generator.MarkdownWriter,
    methods: list[dict[str, Any]],
) -> None:
    if not methods:
        return

    generator.write_header("Methods", 3)

    for method in methods:
        name = method.get("Name", "Unknown")
        return_type = method.get("ReturnType", "")
        signature = method.get("Signature", "")

        generator.write_header(name, 4)

        if return_type:
            generator.write_text(f"Return type: `{return_type}`")
            generator.newline()

        write_signature(generator, signature)

        if method.get("IsConst", False):
            generator.write_text("Const method")
            generator.newline()

        if method.get("Description"):
            write_description(generator, method["Description"])
        else:
            generator.newline()


def generate_operator_docs(
    generator: markdown_generator.MarkdownWriter,
    operators: list[dict[str, Any]],
) -> None:
    if not operators:
        return

    generator.write_header("Operators", 3)

    for operator in operators:
        operator_name = operator.get("Operator", "Unknown")
        return_type = operator.get("ReturnType", "")
        signature = operator.get("Signature", "")

        generator.write_header(operator_name, 4)

        if return_type:
            generator.write_text(f"Return type: `{return_type}`")
            generator.newline()

        write_signature(generator, signature)

        if operator.get("Description"):
            write_description(generator, operator["Description"])
        else:
            generator.newline()


def generate_type_docs(
    generator: markdown_generator.MarkdownWriter,
    as_type: TypeDocumentation,
) -> None:
    generator.write_header(as_type.name, 2)

    if as_type.cpp_type:
        generator.write_text(f"C++ type: `{as_type.cpp_type}`")
        generator.newline()

    if as_type.flags:
        generator.write_text(
            f"Flags: {', '.join(f'`{flag}`' for flag in as_type.flags)}"
        )
        generator.newline()

    write_description(generator, as_type.description)

    generate_property_docs(generator, as_type.properties)
    generate_constructor_docs(generator, as_type.constructors)
    generate_behaviour_docs(generator, as_type.behaviours)
    generate_method_docs(generator, as_type.methods)
    generate_operator_docs(generator, as_type.operators)

    generator.newline()


def generate_function_docs(
    generator: markdown_generator.MarkdownWriter,
    functions: list[DocumentationInfo],
    header: str,
) -> None:
    if not functions:
        return

    generator.write_header(header, 2)

    for function in functions:
        generator.write_header(function.name, 3)

        if function.return_type:
            generator.write_text(f"Return type: `{function.return_type}`")
            generator.newline()

        write_signature(generator, function.signature)
        write_description(generator, function.description)


def generate_enum_docs(
    generator: markdown_generator.MarkdownWriter,
    enums: list[EnumDocumentation],
) -> None:
    if not enums:
        return

    generator.write_header("Enums", 2)

    for enum in enums:
        generator.write_header(enum.name, 3)

        if enum.cpp_type:
            generator.write_text(f"C++ type: `{enum.cpp_type}`")
            generator.newline()

        if enum.values:
            generator.write_text("Values:")
            generator.newline()

            for value in enum.values:
                if isinstance(value, dict):
                    name = value.get("Name", "Unknown")
                    enum_value = value.get("Value")

                    if enum_value is None:
                        generator.write_text(f"- `{name}`")
                    else:
                        generator.write_text(
                            f"- `{name}` = `{enum_value}`"
                        )
                else:
                    generator.write_text(f"- `{value}`")

            generator.newline()


def generate_constant_docs(
    generator: markdown_generator.MarkdownWriter,
    constants: list[dict[str, Any]],
) -> None:
    if not constants:
        return

    generator.write_header("Constants", 2)

    for constant in constants:
        name = constant.get("Name", "Unknown")
        constant_type = constant.get("Type", "")
        value = constant.get("Value", "")

        generator.write_header(name, 3)

        if constant_type:
            generator.write_text(f"Type: `{constant_type}`")
            generator.newline()

        generator.write_text(f"Value: `{value}`")
        generator.newline()

        write_description(
            generator,
            constant.get("Description", ""),
        )


def generate_alias_docs(
    generator: markdown_generator.MarkdownWriter,
    aliases: list[dict[str, Any]],
) -> None:
    if not aliases:
        return

    generator.write_header("Type Aliases", 2)

    for alias in aliases:
        name = alias.get("Name", "Unknown")
        target = alias.get("Target", "")

        generator.write_header(name, 3)

        if target:
            generator.write_text(f"Alias of: `{target}`")
            generator.newline()

        write_description(
            generator,
            alias.get("Description", ""),
        )


def generate_declaration_docs(
    generator: markdown_generator.MarkdownWriter,
    declarations: list[dict[str, Any]],
) -> None:
    if not declarations:
        return

    generator.write_header("Declarations", 2)

    for declaration in declarations:
        name = declaration.get("Name", "Unknown")

        generator.write_header(name, 3)

        write_description(
            generator,
            declaration.get("Description", ""),
        )


def generate_markdown(binding: BindingDocumentation) -> str:
    generator = markdown_generator.MarkdownWriter()

    if binding.global_description:
        generator.write_header("Description", 1)
        generator.write_text(binding.global_description)

    if binding.namespace:
        generator.write_header(binding.namespace, 1)
        generator.newline()

    for as_type in binding.types:
        generate_type_docs(generator, as_type)

    generate_function_docs(
        generator,
        binding.functions,
        "Functions",
    )

    generate_enum_docs(generator, binding.enums)
    generate_constant_docs(generator, binding.constants)
    generate_alias_docs(generator, binding.aliases)
    generate_declaration_docs(generator, binding.declarations)

    generate_function_docs(
        generator,
        binding.class_functions,
        "Functions",
    )

    return generator.build()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Documentation generator to turn CE IDL into Markdown "
            "docs using the documentation data in the IDL"
        )
    )

    parser.add_argument(
        "--idl_dir",
        type=Path,
        required=True,
        help="Directory for the IDL files",
    )
    parser.add_argument(
        "--output_dir",
        type=Path,
        required=True,
        help="Directory to output the Markdown files",
    )

    return parser.parse_args()


def main() -> int:
    args: argparse.Namespace = parse_args()
    files = list(args.idl_dir.rglob("*.idl.yaml"))

    args.output_dir.mkdir(parents=True, exist_ok=True)

    if not files:
        print("No IDL files found in specified IDL dir")
        return 1

    generated_docs: list[Path] = []

    for file in files:
        binding = parse_binding_file(file)

        if not any(
            [
                binding.types,
                binding.functions,
                binding.enums,
                binding.constants,
                binding.aliases,
                binding.declarations,
                binding.class_functions,
            ]
        ):
            print(f"Nothing to create docs out of in {file}")
            continue

        output_docs = generate_markdown(binding)

        output_file = args.output_dir / (
            file.name.removesuffix(".idl.yaml") + ".md"
        )

        output_file.write_text(output_docs, encoding="utf-8")
        generated_docs.append(output_file)

    index_file = args.output_dir / "angelscript_api_index.md"

    with index_file.open("w", encoding="utf-8") as handle:
        handle.write("# AngelScript API Index\n\n")

        for doc in sorted(generated_docs):
            handle.write(f"- [{doc.stem}]({doc.name})\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())