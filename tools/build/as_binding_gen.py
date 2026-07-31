import idl
import generator

def generate_as_type_binding(type: idl.ASType, gen: generator.CodeWriter) -> None:
    flags = " | ".join(
        FLAG_MAP[x]
        for x in type.flags
    )

    gen.push_as_namespace(type.namespace)

    gen.write(
        f'CE_CHECK_AS(mScriptEngine.RegisterObjectType('
        f'"{type.name}", '
        f'sizeof({type.cpp_type}), '
        f'{flags}));'
    )

    for ctor in type.constructors:
        gen.write(
            f'CE_CHECK_AS(mScriptEngine.RegisterObjectBehaviour('
            f'"{type.name}", '
            f'{BEHAVIOUR_MAP["Construct"]}, '
            f'"void f({ctor.signature})", '
            f'{ctor.cpp_function}, '
            f'{CALL_CONV_MAP["CDeclObjLast"]}));'
        )

    for behaviour in type.behaviours:
        declaration = (
            f'void f({behaviour.signature})'
            if behaviour.signature else
            'void f()'
        )

        gen.write(
            f'CE_CHECK_AS(mScriptEngine.RegisterObjectBehaviour('
            f'"{type.name}", '
            f'{BEHAVIOUR_MAP[behaviour.type]}, '
            f'"{declaration}", '
            f'{behaviour.cpp_function}, '
            f'{CALL_CONV_MAP[behaviour.calling_convention]}));'
        )

    for method in type.methods:
        declaration = (
            f'{method.return_type} '
            f'{method.name}'
            f'({method.signature})'
            f'{" const" if method.is_const else ""}'
        )

        gen.write(
            f'CE_CHECK_AS(mScriptEngine.RegisterObjectMethod('
            f'"{type.name}", '
            f'"{declaration}", '
            f'{method.cpp_function}, '
            f'{CALL_CONV_MAP[method.calling_convention]}));'
        )

    for op in type.operators:
        declaration = (
            f'{op.return_type} '
            f'{op.operator}'
            f'({op.signature})'
        )

        gen.write(
            f'CE_CHECK_AS(mScriptEngine.RegisterObjectMethod('
            f'"{type.name}", '
            f'"{declaration}", '
            f'{op.cpp_function}, '
            f'{CALL_CONV_MAP["ThisCall"]}));'
        )

    for prop in type.properties:
        gen.write(
            f'CE_CHECK_AS(mScriptEngine.RegisterObjectProperty('
            f'"{type.name}", '
            f'"{prop.type} {prop.as_member}", '
            f'asOFFSET({type.cpp_type}, {prop.cpp_member})));'
        )

    gen.pop_as_namespace()

def generate_as_function_binding(func: idl.ASFunction, gen: generator.CodeWriter) -> None:
    return

def generate_as_enum_binding(enum: idl.ASEnum, gen: generator.CodeWriter) -> None:
    return

def generate_as_constant(constant: idl.ASConstant, gen: generator.CodeWriter) -> None:
    return