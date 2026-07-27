# Takes a yaml file and generates a C++ binding for angelscript

import argparse
from pathlib import Path

def generate_comment(comment: str) -> str:
    return f"/* {comment} */"

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser("Build helper for CE to turn a yaml registration file into AngelScript bindings")
    parser.add_argument("--source_dir", type=Path, help="Directory for source")
    parser.add_argument("--include_dir", type=Path, help="Directory for include")
    parser.add_argument("--yaml_file", type=Path, help="Path to the yaml file")
    parser.add_argument("--output_dir", type=Path, help="Output directory for the generated header and source")
    return parser.parse_args()

def main() -> int:
    args = parse_args()