#pragma once

#include <cassert>
#include <string>
#include <vector>

#include <angelscript.h>

namespace CEScriptC {
    namespace Detail {
        template <typename T>
        struct VectorBindings {
            using Vector = std::vector<T>;

            static void Construct(Vector* self) {
                new (self) Vector();
            }

            static void ConstructCopy(const Vector&, Vector* self) {
                new (self) Vector();
            }

            static void ConstructSize(asUINT, Vector* self) {
                new (self) Vector();
            }

            static void Destruct(Vector* self) {
                self->~Vector();
            }

            static Vector& Assign(const Vector&, Vector* self) {
                return *self;
            }

            static T& Index(asUINT, Vector*) {
                static T value{};
                return value;
            }

            static const T& IndexConst(asUINT, const Vector*) {
                static const T value{};
                return value;
            }

            static asUINT Size(const Vector*) {
                return 0;
            }

            static bool Empty(const Vector*) {
                return true;
            }

            static void Resize(asUINT, Vector*) {}

            static void Clear(Vector*) {}

            static void PushBack(const T&, Vector*) {}

            static void PopBack(Vector*) {}

            static void Erase(asUINT, Vector*) {}

            static void Insert(asUINT, const T&, Vector*) {}
        };

        template <typename T>
        bool RegisterBehaviour(asIScriptEngine* engine, const char* type_name, asEBehaviours behaviour,
                               const char* declaration, const asSFuncPtr& function) {
            const int result =
                engine->RegisterObjectBehaviour(type_name, behaviour, declaration, function, asCALL_CDECL_OBJLAST);

            return result >= 0;
        }

        template <typename T>
        bool RegisterMethod(asIScriptEngine* engine, const char* type_name, const char* declaration,
                            const asSFuncPtr& function) {
            const int result = engine->RegisterObjectMethod(type_name, declaration, function, asCALL_CDECL_OBJLAST);

            return result >= 0;
        }
    }

    template <typename T>
    bool RegisterVector(asIScriptEngine* engine, const std::string& script_type_name,
                        const std::string& script_element_type_name) {
        assert(engine != nullptr);

        using Vector = std::vector<T>;
        using Bindings = Detail::VectorBindings<T>;

        const std::string copyConstructor = "void f(const " + script_type_name + " &in)";
        const std::string index = script_element_type_name + " &opIndex(uint)";
        const std::string constIndex = "const " + script_element_type_name + " &opIndex(uint) const";
        const std::string assignment = script_type_name + " &opAssign(const " + script_type_name + " &in)";
        const std::string pushBack = "void push_back(const " + script_element_type_name + " &in)";
        const std::string insert = "void insert(uint, const " + script_element_type_name + " &in)";

        int result = engine->RegisterObjectType(script_type_name.c_str(), sizeof(Vector),
                                                asOBJ_VALUE | asGetTypeTraits<Vector>());

        if (result < 0) {
            return false;
        }

        if (!Detail::RegisterBehaviour<T>(engine, script_type_name.c_str(), asBEHAVE_CONSTRUCT, "void f()",
                                          asFUNCTION(Bindings::Construct))) {
            return false;
        }

        if (!Detail::RegisterBehaviour<T>(engine, script_type_name.c_str(), asBEHAVE_CONSTRUCT, copyConstructor.c_str(),
                                          asFUNCTION(Bindings::ConstructCopy))) {
            return false;
        }

        if (!Detail::RegisterBehaviour<T>(engine, script_type_name.c_str(), asBEHAVE_CONSTRUCT, "void f(uint)",
                                          asFUNCTION(Bindings::ConstructSize))) {
            return false;
        }

        if (!Detail::RegisterBehaviour<T>(engine, script_type_name.c_str(), asBEHAVE_DESTRUCT, "void f()",
                                          asFUNCTION(Bindings::Destruct))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), index.c_str(), asFUNCTION(Bindings::Index))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), constIndex.c_str(),
                                       asFUNCTION(Bindings::IndexConst))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), assignment.c_str(),
                                       asFUNCTION(Bindings::Assign))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), "uint size() const",
                                       asFUNCTION(Bindings::Size))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), "bool empty() const",
                                       asFUNCTION(Bindings::Empty))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), "void resize(uint)",
                                       asFUNCTION(Bindings::Resize))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), "void clear()",
                                       asFUNCTION(Bindings::Clear))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), pushBack.c_str(),
                                       asFUNCTION(Bindings::PushBack))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), "void pop_back()",
                                       asFUNCTION(Bindings::PopBack))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), "void erase(uint)",
                                       asFUNCTION(Bindings::Erase))) {
            return false;
        }

        if (!Detail::RegisterMethod<T>(engine, script_type_name.c_str(), insert.c_str(),
                                       asFUNCTION(Bindings::Insert))) {
            return false;
        }

        return true;
    }
}