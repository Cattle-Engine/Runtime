#include "engine/scripting/bindings/accs.hpp"

#include "engine/accs/components/2d_rendering.hpp"
#include "engine/accs/components/3d_rendering.hpp"
#include "engine/accs/components/transform.hpp"

#include <memory>
#include <string_view>
#include <utility>

#include <angelscript.h>
#include <scriptarray/scriptarray.h>

#include "engine/accs/accs.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/instance.hpp"
#include "engine/scripting/bindings/binding_macros.hpp"

namespace CE::Scripting::Bindings {
    namespace {
        class ScriptComponent final : public ACCS::IComponent {
          public:
            explicit ScriptComponent(asIScriptObject& object) : mObject(&object) {
                mObject->AddRef();
                asOBJ_VALUE;
            }

            ~ScriptComponent() override {
                mObject->Release();
            }

            asIScriptObject& GetObject() const { return *mObject; }

          private:
            asIScriptObject* mObject;
        };

        class ScriptSystem final : public ACCS::ISystem {
          public:
            ScriptSystem(asIScriptEngine& engine, asIScriptObject& object, asIScriptFunction& update)
                : mEngine(engine), mObject(&object), mUpdate(&update) {
                mObject->AddRef();
                mUpdate->AddRef();
            }

            ~ScriptSystem() override {
                mUpdate->Release();
                mObject->Release();
            }

            void Update(float dt, ACCS::FrameUpdateType type, ACCS::World&, ACCS::ACCS&) override {
                asIScriptContext* context = mEngine.CreateContext();
                if (!context) {
                    CE_LOG(LogLevel::Error, "[AngelScript] Failed to create ACCS system context");
                    return;
                }

                const int prepare = context->Prepare(mUpdate);
                if (prepare >= 0) {
                    context->SetObject(mObject);
                    context->SetArgFloat(0, dt);
                    context->SetArgDWord(1, static_cast<asDWORD>(type));
                    const int result = context->Execute();
                    if (result != asEXECUTION_FINISHED) {
                        CE_LOG(LogLevel::Error,
                               "[AngelScript] ACCS system '{}' failed: {}",
                               mUpdate->GetDeclaration(),
                               context->GetExceptionString() ? context->GetExceptionString() : "execution error");
                    }
                } else {
                    CE_LOG(LogLevel::Error, "[AngelScript] Failed to prepare ACCS system update");
                }
                context->Release();
            }

          private:
            asIScriptEngine& mEngine;
            asIScriptObject* mObject;
            asIScriptFunction* mUpdate;
        };

        ACCS::ACCS& GetACCS(Runtime& runtime) {
            return runtime.mInstance.GetACCS();
        }

        ACCS::ActorHandle CreateActor(Runtime& runtime) {
            return GetACCS(runtime).world.CreateActor();
        }

        bool DestroyActor(const ACCS::ActorHandle& actor, Runtime& runtime) {
            return GetACCS(runtime).world.DestroyActor(actor);
        }

        bool IsActorAlive(const ACCS::ActorHandle& actor, Runtime& runtime) {
            for (const auto& candidate : GetACCS(runtime).world.GetActors()) {
                if (candidate == actor) {
                    return true;
                }
            }
            return false;
        }

        ACCS::ComponentID AddComponent(const ACCS::ActorHandle& actor, asIScriptObject* object, Runtime& runtime) {
            if (!object) {
                return {};
            }
            try {
                auto component = std::make_shared<ScriptComponent>(*object);
                return GetACCS(runtime).world.AddComponent(actor, object->GetObjectType(), std::move(component));
            } catch (const std::exception& exception) {
                CE_LOG(LogLevel::Error, "[AngelScript] Failed to add ACCS component: {}", exception.what());
                return {};
            }
        }

        bool RemoveComponent(const ACCS::ComponentID& component, Runtime& runtime) {
            return GetACCS(runtime).world.RemoveComponent(component);
        }

        bool HasComponent(const ACCS::ComponentID& component, Runtime& runtime) {
            return GetACCS(runtime).world.HasComponent(component);
        }

        asIScriptObject* GetComponent(const ACCS::ComponentID& component, Runtime& runtime) {
            // Components added from C++ are intentionally not exposed as an
            // IComponent handle; script components retain their concrete type.
            auto* script_component = dynamic_cast<ScriptComponent*>(
                GetACCS(runtime).world.GetComponent(component));
            if (!script_component) {
                return nullptr;
            }
            asIScriptObject& object = script_component->GetObject();
            object.AddRef();
            return &object;
        }

        template <typename T>
        CScriptArray* MakeArray(
            Runtime& runtime,
            std::string_view declaration,
            const std::vector<T>& values) {
            asITypeInfo* type = runtime.GetScriptEngine().GetTypeInfoByDecl(declaration.data());
            if (!type) {
                return nullptr;
            }

            CScriptArray* array = CScriptArray::Create(type, static_cast<asUINT>(values.size()));
            for (asUINT index = 0; index < values.size(); ++index) {
                *static_cast<T*>(array->At(index)) = values[index];
            }
            return array;
        }

        CScriptArray* GetActors(Runtime& runtime) {
            return MakeArray(runtime, "array<CE::ACCS::ActorHandle>", GetACCS(runtime).world.GetActors());
        }

        CScriptArray* GetComponents(const ACCS::ActorHandle& actor, Runtime& runtime) {
            return MakeArray(runtime, "array<CE::ACCS::ComponentID>", GetACCS(runtime).world.GetComponents(actor));
        }

        void RegisterSystem(ACCS::FrameUpdateType types, asIScriptObject* object, Runtime& runtime) {
            if (!object || types == ACCS::FrameUpdateType::None) {
                return;
            }

            asIScriptFunction* update = object->GetObjectType()->GetMethodByName("Update");
            if (!update) {
                CE_LOG(LogLevel::Error, "[AngelScript] ACCS system does not implement Update");
                return;
            }

            try {
                auto system = std::make_unique<ScriptSystem>(runtime.GetScriptEngine(), *object, *update);
                runtime.mInstance.GetACCS().system_manager.RegisterSystem(types, std::move(system));
            } catch (const std::exception& exception) {
                CE_LOG(LogLevel::Error, "[AngelScript] Failed to register ACCS system: {}", exception.what());
            }
        }

        template <typename T>
        T* GetNativeByID(const ACCS::ComponentID& id, Runtime& runtime) {
            // World::GetComponent<T>(id) checks the stored type key, so a wrong ID returns nullptr.
            return GetACCS(runtime).world.GetComponent<T>(id);
        }

        template <typename T>
        T* GetNativeByActor(const ACCS::ActorHandle& actor, Runtime& runtime) {
            return GetACCS(runtime).world.GetComponent<T>(actor);
        }

        template <typename T, typename BindMembers>
        bool RegisterNativeComponent(
            asIScriptEngine& engine,
            Runtime& runtime,
            NativeTypeRegistry& registry,
            const std::string& name,
            BindMembers&& bindMembers
        ) {
            CE_CHECK_AS(engine.RegisterObjectType(name.c_str(), 0, asOBJ_REF | asOBJ_NOCOUNT));

            if (!bindMembers(name.c_str())) {
                return false;
            }

            registry.Register<T>(name);

            const std::string getter = "Get" + name;

            CE_CHECK_AS(engine.RegisterGlobalFunction(
                (name + "@ " + getter + "(const ActorHandle &in)").c_str(),
                asFUNCTION(GetNativeByActor<T>),
                asCALL_CDECL_OBJLAST, &runtime));

            CE_CHECK_AS(engine.RegisterGlobalFunction(
                (name + "@ " + getter + "(const ComponentID &in)").c_str(),
                asFUNCTION(GetNativeByID<T>),
                asCALL_CDECL_OBJLAST, &runtime));

            return true;
        }
    } // namespace

    ACCS::ComponentID ACCSBinding::AddNativeComponent(
        const ACCS::ActorHandle& actor,
        const std::string& type
    ) {
        try {
            return native_types.Create(
                type,
                GetACCS(mRuntime).world,
                actor
            );
        } catch (const std::exception& exception) {
            CE_LOG(LogLevel::Error,
                "[ACCS] Failed to add native component '{}': {}",
                type,
                exception.what());
            return {};
        }
    }

    bool ACCSBinding::RegisterBindings() {
        mScriptEngine.SetDefaultNamespace("CE::ACCS");

        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "ActorHandle CreateActor()", asFUNCTIONPR(CreateActor, (Runtime&), ACCS::ActorHandle),
            asCALL_CDECL_OBJLAST, &mRuntime));
        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "bool DestroyActor(const ActorHandle &in)", asFUNCTIONPR(DestroyActor, (const ACCS::ActorHandle&, Runtime&), bool),
            asCALL_CDECL_OBJLAST, &mRuntime));
        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "bool IsActorAlive(const ActorHandle &in)", asFUNCTIONPR(IsActorAlive, (const ACCS::ActorHandle&, Runtime&), bool),
            asCALL_CDECL_OBJLAST, &mRuntime));
        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "ComponentID AddComponent(const ActorHandle &in, IComponent@)", asFUNCTIONPR(AddComponent, (const ACCS::ActorHandle&, asIScriptObject*, Runtime&), ACCS::ComponentID),
            asCALL_CDECL_OBJLAST, &mRuntime));
        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "bool RemoveComponent(const ComponentID &in)", asFUNCTIONPR(RemoveComponent, (const ACCS::ComponentID&, Runtime&), bool),
            asCALL_CDECL_OBJLAST, &mRuntime));
        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "bool HasComponent(const ComponentID &in)", asFUNCTIONPR(HasComponent, (const ACCS::ComponentID&, Runtime&), bool),
            asCALL_CDECL_OBJLAST, &mRuntime));
        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "IComponent@ GetComponent(const ComponentID &in)", asFUNCTIONPR(GetComponent, (const ACCS::ComponentID&, Runtime&), asIScriptObject*),
            asCALL_CDECL_OBJLAST, &mRuntime));
        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "array<ActorHandle>@ GetActors()", asFUNCTIONPR(GetActors, (Runtime&), CScriptArray*),
            asCALL_CDECL_OBJLAST, &mRuntime));
        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "array<ComponentID>@ GetComponents(const ActorHandle &in)", asFUNCTIONPR(GetComponents, (const ACCS::ActorHandle&, Runtime&), CScriptArray*),
            asCALL_CDECL_OBJLAST, &mRuntime));
        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "void RegisterSystem(FrameUpdateType, ISystem@)", asFUNCTIONPR(RegisterSystem, (ACCS::FrameUpdateType, asIScriptObject*, Runtime&), void),
            asCALL_CDECL_OBJLAST, &mRuntime));

        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "ComponentID AddNativeComponent(const ActorHandle &in, const string &in)",
            asMETHODPR(
                ACCSBinding,
                AddNativeComponent,
                (const ACCS::ActorHandle&, const std::string&),
                ACCS::ComponentID
            ),
            asCALL_THISCALL_ASGLOBAL,
            this
        ));

        mScriptEngine.SetDefaultNamespace("");
        return true;

        CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction(
            "ComponentID AddNativeComponent(const ActorHandle &in, const string &in)",
            asMETHODPR(ACCSBinding, AddNativeComponent,
                    (const ACCS::ActorHandle&, const std::string&), ACCS::ComponentID),
            asCALL_THISCALL_ASGLOBAL, this));

        if (!RegisterNativeComponent<ACCS::Components::Transform>(
                mScriptEngine, mRuntime, native_types, "Transform",
                [&](const char* n) {
                    CE_CHECK_AS(mScriptEngine.RegisterObjectProperty(
                        n, "Transform3D transform_3d",
                        asOFFSET(ACCS::Components::Transform, transform_3d)));
                    CE_CHECK_AS(mScriptEngine.RegisterObjectProperty(
                        n, "Transform2D transform_2d",
                        asOFFSET(ACCS::Components::Transform, transfrom_2d)));
                    return true;
                })) {
            return false;
        }

        mScriptEngine.SetDefaultNamespace("");
        return true;
    }
} // namespace CE::Scripting::Bindings
