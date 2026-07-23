#pragma once
// Shared macros used by bindings


// This MUST only be called from an IScriptBinding as it uses members only inside that!
#define CE_REGISTER_TYPE(name, size, flags) \
do { \
    if (mScriptEngine.RegisterObjectType((name), (size), (flags)) < 0) { \
        return false; \
    } \
} while (false)

// This also MUST only be called from IScriptBinding
#define CE_REGISTER_GLOBAL(decl, method)                                                                               \
do {                                                                                                               \
    if (mScriptEngine.RegisterGlobalFunction(decl, asMETHOD(Runtime, method), asCALL_THISCALL_ASGLOBAL, this) <   \
        0) {                                                                                                       \
            return false;                                                                                              \
        }                                                                                                              \
} while (false)