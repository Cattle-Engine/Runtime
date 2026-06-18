#pragma once

#define CE_REGISTER_TYPE(name, size, flags) \
    do { \
        if (mScriptEngine->RegisterObjectType( \
                name, \
                size, \
                flags) < 0) { \
            return false; \
        } \
    } while (false)

#define CE_REGISTER_GLOBAL(decl, method) \
    do { \
        if (mScriptEngine->RegisterGlobalFunction( \
                decl, \
                asMETHOD(Runtime, method), \
                asCALL_THISCALL_ASGLOBAL, \
                this) < 0) { \
            return false; \
        } \
    } while (false)
