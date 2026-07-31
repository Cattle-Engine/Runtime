import idl
import generator

def generate_as_type_binding(type: idl.ASType, gen: generator.CodeWriter) -> None:
    flags = " | ".join(
        FLAG_MAP[x]
        for x in type.flags
    )
    
    gen.push_as_namespace(type.namespace)

    gen.write(
        f'mScriptEngine.RegisterObjectType('
        f'"{type.name}", '
        f'sizeof({type.cpp_type}), '
        f'{flags});'
    )

    gen.pop_as_namespace()

def generate_as_function_binding(func: idl.ASFunction, gen: generator.CodeWriter) -> None:
    return

def generate_as_enum_binding(enum: idl.ASEnum, gen: generator.CodeWriter) -> None:
    return

def generate_as_constant(constant: idl.ASConstant, gen: generator.CodeWriter) -> None:
    return