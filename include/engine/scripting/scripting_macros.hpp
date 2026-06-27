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

#define CE_CONCAT_INNER(a, b) a##b
#define CE_CONCAT(a, b) CE_CONCAT_INNER(a, b)

#define CE_REGISTER_NAME_REGISTRY_GLOBAL(registry, cppType, scriptType, namespaceName) \
    CE_REGISTER_NAME_REGISTRY_GLOBAL_IMPL(registry, cppType, scriptType, namespaceName, __COUNTER__)

#define CE_REGISTER_NAME_REGISTRY_GLOBAL_WRAPPER(registry, cppType, scriptType, namespaceName, handleMember) \
    CE_REGISTER_NAME_REGISTRY_GLOBAL_WRAPPER_IMPL(registry, cppType, scriptType, namespaceName, handleMember, __COUNTER__)

#define CE_REGISTER_NAME_REGISTRY_GLOBAL_IMPL(registry, cppType, scriptType, namespaceName, id) \
    do { \
        struct CE_CONCAT(_ce_registry_wrapper_, id) { \
            std::remove_reference_t<decltype(registry)>* Registry; \
            \
            static bool Exists(std::string_view name) { \
                return GetRegistry()->Exists(name); \
            } \
            \
            static void Add(std::string_view name, const cppType& value) { \
                GetRegistry()->Add(std::string(name), value); \
            } \
            \
            static const cppType& Get(std::string_view name) { \
                return GetRegistry()->Get(std::string(name)); \
            } \
            \
            static void Remove(std::string_view name) { \
                GetRegistry()->Remove(std::string(name)); \
            } \
            \
            static void Clear() { \
                GetRegistry()->Clear(); \
            } \
            \
            /* Return a reference to the pointer so it can be assigned */ \
            static std::remove_reference_t<decltype(registry)>*& GetRegistry() { \
                static std::remove_reference_t<decltype(registry)>* reg = nullptr; \
                return reg; \
            } \
        }; \
        \
        CE_CONCAT(_ce_registry_wrapper_, id)::GetRegistry() = &registry; \
        \
        mScriptEngine->SetDefaultNamespace(namespaceName); \
        \
        if (mScriptEngine->RegisterGlobalFunction( \
                "bool Exists(const string &in)", \
                asFUNCTION(CE_CONCAT(_ce_registry_wrapper_, id)::Exists), \
                asCALL_CDECL) < 0) { \
            return false; \
        } \
        \
        if (mScriptEngine->RegisterGlobalFunction( \
                "void Add(const string &in, const " scriptType " &in)", \
                asFUNCTION(CE_CONCAT(_ce_registry_wrapper_, id)::Add), \
                asCALL_CDECL) < 0) { \
            return false; \
        } \
        \
        if (mScriptEngine->RegisterGlobalFunction( \
                "const " scriptType " &Get(const string &in)", \
                asFUNCTION(CE_CONCAT(_ce_registry_wrapper_, id)::Get), \
                asCALL_CDECL) < 0) { \
            return false; \
        } \
        \
        if (mScriptEngine->RegisterGlobalFunction( \
                "void Remove(const string &in)", \
                asFUNCTION(CE_CONCAT(_ce_registry_wrapper_, id)::Remove), \
                asCALL_CDECL) < 0) { \
            return false; \
        } \
        \
        if (mScriptEngine->RegisterGlobalFunction( \
                "void Clear()", \
                asFUNCTION(CE_CONCAT(_ce_registry_wrapper_, id)::Clear), \
                asCALL_CDECL) < 0) { \
            return false; \
        } \
        \
        mScriptEngine->SetDefaultNamespace(""); \
    } while (false)

#define CE_REGISTER_NAME_REGISTRY_GLOBAL_WRAPPER_IMPL(registry, cppType, scriptType, namespaceName, handleMember, id) \
    do { \
        struct CE_CONCAT(_ce_registry_wrapper_, id) { \
            static bool Exists(std::string_view name) { \
                return GetRegistry()->Exists(name); \
            } \
            \
            static void Add(std::string_view name, const cppType& value) { \
                GetRegistry()->Add(std::string(name), value.handleMember); \
            } \
            \
            static const cppType& Get(std::string_view name) { \
                static cppType cachedValue{}; \
                cachedValue = cppType{ GetRegistry()->Get(std::string(name)) }; \
                return cachedValue; \
            } \
            \
            static void Remove(std::string_view name) { \
                GetRegistry()->Remove(std::string(name)); \
            } \
            \
            static void Clear() { \
                GetRegistry()->Clear(); \
            } \
            \
            static std::remove_reference_t<decltype(registry)>*& GetRegistry() { \
                static std::remove_reference_t<decltype(registry)>* reg = nullptr; \
                return reg; \
            } \
        }; \
        \
        CE_CONCAT(_ce_registry_wrapper_, id)::GetRegistry() = &registry; \
        \
        mScriptEngine->SetDefaultNamespace(namespaceName); \
        \
        if (mScriptEngine->RegisterGlobalFunction( \
                "bool Exists(const string &in)", \
                asFUNCTION(CE_CONCAT(_ce_registry_wrapper_, id)::Exists), \
                asCALL_CDECL) < 0) { \
            return false; \
        } \
        \
        if (mScriptEngine->RegisterGlobalFunction( \
                "void Add(const string &in, const " scriptType " &in)", \
                asFUNCTION(CE_CONCAT(_ce_registry_wrapper_, id)::Add), \
                asCALL_CDECL) < 0) { \
            return false; \
        } \
        \
        if (mScriptEngine->RegisterGlobalFunction( \
                "const " scriptType " &Get(const string &in)", \
                asFUNCTION(CE_CONCAT(_ce_registry_wrapper_, id)::Get), \
                asCALL_CDECL) < 0) { \
            return false; \
        } \
        \
        if (mScriptEngine->RegisterGlobalFunction( \
                "void Remove(const string &in)", \
                asFUNCTION(CE_CONCAT(_ce_registry_wrapper_, id)::Remove), \
                asCALL_CDECL) < 0) { \
            return false; \
        } \
        \
        if (mScriptEngine->RegisterGlobalFunction( \
                "void Clear()", \
                asFUNCTION(CE_CONCAT(_ce_registry_wrapper_, id)::Clear), \
                asCALL_CDECL) < 0) { \
            return false; \
        } \
        \
        mScriptEngine->SetDefaultNamespace(""); \
    } while (false)