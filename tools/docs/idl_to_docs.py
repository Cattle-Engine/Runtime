import argparse
from pathlib import Path
import markdown_generator

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        "Documentation generator to turn CE idl into markdown docs using the documentation data in the idl"
    )

    parser.add_argument("--idl_dir", type=Path, required=True, help="Directory for the idl files")
    parser.add_argument("--output_dir", type=Path, required=True, help="Directory to output the markdown files")
    return parser.parse_args()

def main() -> int:
    args: argparse.Namespace = parse_args()
    files = list(args.idl_dir.rglob("*idl.yaml"))

    if not files:
        print("No idl files found in specficed idl dir")
        return 1

    return 0

if __name__ == "__main__":
    raise SystemExit(main())