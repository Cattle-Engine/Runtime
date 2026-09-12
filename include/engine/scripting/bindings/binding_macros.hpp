#pragma once
// Shared macros used by bindings

#include "engine/common/tracelog.hpp"

// The below functions must only be called from a function that returns false and is in IScriptBinding
#define CE_REGISTER_TYPE(name, size, flags)                                                                            \
    do {                                                                                                               \
        const int result = mScriptEngine.RegisterObjectType((name), (size), (flags));                                  \
        if (result < 0) {                                                                                              \
            CE_LOG(CE::LogLevel::Error,                                                                                \
                   "AngelScript RegisterObjectType failed: name='{}', size={}, flags={}, result={}",                   \
                   (name), (size), static_cast<int>(flags), result);                                                   \
            return false;                                                                                              \
        }                                                                                                              \
    } while (false)

#define CE_REGISTER_GLOBAL(type, obj, decl, method)                                                                     \
    do {                                                                                                                \
        const int result =                                                                                              \
            mScriptEngine.RegisterGlobalFunction(decl, asMETHOD(type, method), asCALL_THISCALL_ASGLOBAL, obj);          \
        if (result < 0) {                                                                                               \
            CE_LOG(CE::LogLevel::Error,                                                                                 \
                   "AngelScript RegisterGlobalFunction failed: declaration='{}', result={}",                            \
                   (decl), result);                                                                                     \
            return false;                                                                                               \
        }                                                                                                               \
    } while (false)

#define CE_REGISTER_OBJECT_BEHAVIOUR(obj, behaviour, declaration, func_ptr, call_conv)                                 \
    do {                                                                                                               \
        const int result =                                                                                             \
            mScriptEngine.RegisterObjectBehaviour(obj, behaviour, declaration, func_ptr, call_conv);                   \
        if (result < 0) {                                                                                              \
            CE_LOG(CE::LogLevel::Error,                                                                                \
                   "AngelScript RegisterObjectBehaviour failed: object='{}', declaration='{}', behaviour={}, result={}", \
                   (obj), (declaration), static_cast<int>(behaviour), result);                                         \
            return false;                                                                                              \
        }                                                                                                               \
    } while (false)

#define CE_REGISTER_OBJECT_PROPERTY(obj, decl, byte_offset)                                                           \
    do {                                                                                                              \
        const int result = mScriptEngine.RegisterObjectProperty(obj, decl, byte_offset);                              \
        if (result < 0) {                                                                                             \
            CE_LOG(CE::LogLevel::Error,                                                                               \
                   "AngelScript RegisterObjectProperty failed: object='{}', declaration='{}', offset={}, result={}",  \
                   (obj), (decl), (byte_offset), result);                                                             \
            return false;                                                                                             \
        }                                                                                                             \
    } while (false)

#define CE_REGISTER_OBJECT_METHOD(obj, declaration, func_ptr, call_conv)                                              \
    do {                                                                                                              \
        const int result = mScriptEngine.RegisterObjectMethod(obj, declaration, func_ptr, call_conv);                 \
        if (result < 0) {                                                                                             \
            CE_LOG(CE::LogLevel::Error,                                                                               \
                   "AngelScript RegisterObjectMethod failed: object='{}', declaration='{}', result={}",               \
                   (obj), (declaration), result);                                                                     \
            return false;                                                                                             \
        }                                                                                                             \
    } while (false)

#define CE_CHECK_AS(call)                                                                                              \
    do {                                                                                                               \
        const int result = (call);                                                                                     \
        if (result < 0) {                                                                                              \
            CE_LOG(CE::LogLevel::Error,                                                                                \
                   "AngelScript call failed: expression='{}', result={}",                                              \
                   #call, result);                                                                                     \
            return false;                                                                                              \
        }                                                                                                              \
    } while (false)