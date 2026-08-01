import random
from typing import Final
from dataclasses import dataclass, field

ANGELSCRIPT_CLASS_INCLUDE: Final = "engine/scripting/bindings/script_binding_class.hpp"
ANGELSCRIPT_MACRO_INCUDE: Final = "engine/scripting/bindings/binding_macros.hpp"
ANGELSCRIPT_MACRO_INCLUDE: Final = ANGELSCRIPT_MACRO_INCUDE

FLAG_MAP = {
    # Object kinds
    "Value": "asOBJ_VALUE",
    "Reference": "asOBJ_REF",

    # Object behaviour
    "GarbageCollected": "asOBJ_GC",
    "NoCount": "asOBJ_NOCOUNT",
    "Scoped": "asOBJ_SCOPED",
    "Template": "asOBJ_TEMPLATE",
    "Shared": "asOBJ_SHARED",
    "NoInherit": "asOBJ_NOINHERIT",

    # Memory/layout hints
    "POD": "asOBJ_POD",

    # Application primitive types
    "AppPrimitive": "asOBJ_APP_PRIMITIVE",
    "AppFloat": "asOBJ_APP_FLOAT",
    "AppArray": "asOBJ_APP_ARRAY",

    # Application class types
    "AppClass": "asOBJ_APP_CLASS",
    "AppClassConstructor": "asOBJ_APP_CLASS_CONSTRUCTOR",
    "AppClassDestructor": "asOBJ_APP_CLASS_DESTRUCTOR",
    "AppClassAssignment": "asOBJ_APP_CLASS_ASSIGNMENT",
    "AppClassCopyConstructor": "asOBJ_APP_CLASS_COPY_CONSTRUCTOR",

    # Application class combinations
    "AppClassCDA": "asOBJ_APP_CLASS_CDA",
    "AppClassCDAK": "asOBJ_APP_CLASS_CDAK",

    # Automatic application class detection
    "AppClassAllInts": "asOBJ_APP_CLASS_ALLINTS",
    "AppClassAllFloats": "asOBJ_APP_CLASS_ALLFLOATS",

    # Alignment
    "AppClassAlign8": "asOBJ_APP_CLASS_ALIGN8",
    "AppClassAlign16": "asOBJ_APP_CLASS_ALIGN16",

    # Additional constructors
    "AppClassMoreConstructors": "asOBJ_APP_CLASS_MORE_CONSTRUCTORS",
}

BEHAVIOUR_MAP = {
    "Construct": "asBEHAVE_CONSTRUCT",
    "Constructor": "asBEHAVE_CONSTRUCT",
    "Factory": "asBEHAVE_FACTORY",
    "Destruct": "asBEHAVE_DESTRUCT",
    "Destructor": "asBEHAVE_DESTRUCT",
    "AddRef": "asBEHAVE_ADDREF",
    "Release": "asBEHAVE_RELEASE",
    "CopyConstruct": "asBEHAVE_COPY_CONSTRUCT",
    "Init": "asBEHAVE_INIT",
}

CALL_CONV_MAP = {
    "CDecl": "asCALL_CDECL",
    "CDeclObjFirst": "asCALL_CDECL_OBJFIRST",
    "CDeclObjLast": "asCALL_CDECL_OBJLAST",
    "ThisCall": "asCALL_THISCALL",
    "ThisCallAsGlobal": "asCALL_THISCALL_ASGLOBAL",
    "Generic": "asCALL_GENERIC",
}

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

    def begin_struct(self, name: str):
        self.begin_block(f"struct {name}")

    def end_struct(self):
        self.end_block(";")

    def push_as_namespace(self, ns: str) -> None:
        if ns:
            self.write(f'mScriptEngine.SetDefaultNamespace("{ns}");')

    def pop_as_namespace(self, ns: str = "") -> None:
        self.write(f'mScriptEngine.SetDefaultNamespace("{ns}");')

    def build(self) -> str:
        return "\n".join(self._lines)
