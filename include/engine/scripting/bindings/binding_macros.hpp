#pragma once
// Shared macros used by bindings


// The below functions must only be called from a function that returns false and is in IScriptBinding
#define CE_REGISTER_TYPE(name, size, flags)                                                     \
do {                                                                                            \
    if (mScriptEngine.RegisterObjectType((name), (size), (flags)) < 0) {                        \
        return false;                                                                           \
    }                                                                                           \
} while (false)

#define CE_REGISTER_GLOBAL(type, obj, decl, method)                                             \
do {                                                                                            \
    if (mScriptEngine.RegisterGlobalFunction(                                                   \
            decl,                                                                               \
            asMETHOD(type, method),                                                             \
            asCALL_THISCALL_ASGLOBAL,                                                           \
            obj) < 0) {                                                                         \
        return false;                                                                           \
    }                                                                                           \
} while (false)

#define CE_REGISTER_OBJECT_BEHAVIOUR(obj, behaviour, declaration, func_ptr, call_conv)          \
do {                                                                                            \
    if (mScriptEngine.RegisterObjectBehaviour(                                                  \
            obj,                                                                                \
            behaviour,                                                                          \
            declaration,                                                                        \
            func_ptr,                                                                           \
            call_conv                                                                           \
        ) < 0) {                                                                                \
        return false;                                                                           \
    }                                                                                           \
} while (false)

#define CE_REGISTER_OBJECT_PROPERTY(obj, decl, byte_offset)                                     \
do {                                                                                            \
    if (mScriptEngine.RegisterObjectProperty(                                                   \
            obj,                                                                                \
            decl,                                                                               \
            byte_offset                                                                         \
        ) < 0) {                                                                                \
        return false;                                                                           \
    }                                                                                           \
} while (false)                 

#define CE_REGISTER_OBJECT_METHOD(obj, declaration, func_ptr, call_conv)                        \
do {                                                                                            \
    if (mScriptEngine.RegisterObjectMethod(                                                     \
            obj,                                                                                \
            declaration,                                                                        \
            func_ptr,                                                                           \
            call_conv                                                                           \
        ) < 0) {                                                                                \
        return false;                                                                           \
    }                                                                                           \
} while (false)

#define CE_CHECK_AS(call)                                                                       \
do {                                                                                            \
    if ((call) != 0) {                                                                          \
        return false;                                                                           \
    }                                                                                           \
} while (false)
