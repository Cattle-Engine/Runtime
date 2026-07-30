import random
from typing import Final
from dataclasses import dataclass, field

ANGELSCRIPT_CLASS_INCLUDE: Final = "engine/scripting/bindings/script_binding_class.hpp"

def generate_symbol_name() -> str:
    return f"_CE_GENERATED_{random.getrandbits(64):016X}"

def generate_comment(comment: str) -> str:
    return f"/* {comment} */"

@dataclass
class CodeWriter:
    _lines: list[str] = field(default_factory=list)
    _indent: int = 0
    indent_string: str = "    "

    @property
    def line(self) -> int:
        return len(self._lines) + 1

    def write(self, text: str = "") -> None:
        if text:
            self._lines.append(f"{self.indent_string * self._indent}{text}")
        else:
            self._lines.append("")

    def indent(self) -> None:
        self._indent += 1

    def dedent(self) -> None:
        if self._indent == 0:
            raise RuntimeError("Cannot dedent below zero")
        self._indent -= 1

    def begin_block(self, header: str) -> None:
        self.write(f"{header} {{")
        self.indent()

    def end_block(self, suffix: str = "") -> None:
        self.dedent()
        self.write(f"}}{suffix}")

    def begin_namespace(self, name: str) -> None:
        self.begin_block(f"namespace {name}")

    def begin_class(self, name: str, inherit: str | None = None) -> None:
        if inherit:
            self.begin_block(f"class {name} : {inherit}")
        else:
            self.begin_block(f"class {name}")

    def begin_function(self, signature: str) -> None:
        self.begin_block(signature)

    def end_class(self) -> None:
        self.end_block(";")

    def build(self) -> str:
        return "\n".join(self._lines)