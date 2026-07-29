# Takes a yaml file and generates a C++ binding for angelscript
import argparse
import yaml
from pathlib import Path
from dataclasses import dataclass, field

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

def generate_comment(comment: str) -> str:
    return f"/* {comment} */"

def generate_header_list(headers: list) -> str:
    return "\n".join(f'#include "{header}"' for header in headers)

def generate_cpp_source(yaml_data) -> str:
    return ""

def generate_cpp_header(yaml_data) -> str:
    return ""

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser("Build helper for CE to turn a yaml registration file into AngelScript bindings")
    parser.add_argument("--source_dir", type=Path, help="Directory for source")
    parser.add_argument("--include_dir", type=Path, help="Directory for include")
    parser.add_argument("--yaml_file", type=Path, help="Path to the yaml file")
    parser.add_argument("--output_dir", type=Path, help="Output directory for the generated header and source")
    return parser.parse_args()

def main() -> int:
    args = parse_args()

    with open(args.yaml_file) as f:
        data = yaml.safe_load(f)

    includes: str = generate_header_list(data["RequiredCppHeaders"]) # Combined with generated_source
    generated_source: str = generate_cpp_source(data)

    source: str = includes + "\n" + generated_source

    return 0

if __name__ == "__main__":
    raise SystemExit(main())