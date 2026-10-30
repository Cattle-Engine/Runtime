#include "engine/scripting/bindings/accs.hpp"

#include "engine/accs/components/2d_rendering.hpp"
#include "engine/accs/components/3d_rendering.hpp"
#include "engine/accs/components/transform.hpp"
#include "engine/accs/world_file.hpp"

#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <angelscript.h>
#include <scriptarray/scriptarray.h>

#include "engine/accs/accs.hpp"
#include "engine/accs/serialization_context.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/instance.hpp"
#include "engine/scripting/bindings/binding_macros.hpp"

namespace CE::Scripting::Bindings {
    namespace {
        using ACCS::ActorHandle;
        using ACCS::ComponentID;
        using CECommon::FS::BinaryReader;
        using CECommon::FS::BinaryWriter;

        // Keeps the default namespace set for the duration of a scope, and
        // always restores the global namespace, including on early returns.
        class NamespaceScope {
          public:
            NamespaceScope(asIScriptEngine& engine, const char* name) : mEngine(engine) {
                mEngine.SetDefaultNamespace(name);
            }
            ~NamespaceScope() { mEngine.SetDefaultNamespace(""); }

            NamespaceScope(const NamespaceScope&) = delete;
            NamespaceScope& operator=(const NamespaceScope&) = delete;

          private:
            asIScriptEngine& mEngine;
        };

        class ScriptComponent final : public ACCS::IComponent {
          public:
            explicit ScriptComponent(asIScriptObject& object) : mObject(&object) {
                mObject->AddRef();
            }

            ~ScriptComponent() override {
                mObject->Release();
            }

            asIScriptObject& GetObject() const { return *mObject; }

          private:
            asIScriptObject* mObject;
        };
        
        enum class PropKind : uint8_t {
            Bool = 1,
            Signed = 2,   // int8/16/32/64 and enums
            Unsigned = 3, // uint8/16/32/64
            Float = 4,
            Double = 5,
            String = 6,   // assumes the script `string` is std::string
            Actor = 7     // CE::ACCS::ActorHandle, remapped through the contexts
        };

        struct ScriptTypeIds {
            int string_id = -1;
            int actor_id = -1;
        };

        std::optional<PropKind> ClassifyProperty(
            asIScriptEngine& engine, int type_id, const ScriptTypeIds& ids) {
            if (type_id & asTYPEID_OBJHANDLE) {
                return std::nullopt;
            }

            switch (type_id) {
                case asTYPEID_BOOL: return PropKind::Bool;
                case asTYPEID_INT8:
                case asTYPEID_INT16:
                case asTYPEID_INT32:
                case asTYPEID_INT64: return PropKind::Signed;
                case asTYPEID_UINT8:
                case asTYPEID_UINT16:
                case asTYPEID_UINT32:
                case asTYPEID_UINT64: return PropKind::Unsigned;
                case asTYPEID_FLOAT: return PropKind::Float;
                case asTYPEID_DOUBLE: return PropKind::Double;
                default: break;
            }

            if (type_id == ids.string_id) {
                return PropKind::String;
            }
            if (type_id == ids.actor_id) {
                return PropKind::Actor;
            }

            const asITypeInfo* info = engine.GetTypeInfoById(type_id);
            if (info && (info->GetFlags() & asOBJ_ENUM)) {
                return PropKind::Signed;
            }
            return std::nullopt;
        }

        int64_t LoadSigned(int type_id, const void* p) {
            switch (type_id) {
                case asTYPEID_INT8: return *static_cast<const int8_t*>(p);
                case asTYPEID_INT16: return *static_cast<const int16_t*>(p);
                case asTYPEID_INT64: return *static_cast<const int64_t*>(p);
                default: return *static_cast<const int32_t*>(p); // int32 and enums
            }
        }

        void StoreSigned(int type_id, void* p, int64_t v) {
            switch (type_id) {
                case asTYPEID_INT8: *static_cast<int8_t*>(p) = static_cast<int8_t>(v); break;
                case asTYPEID_INT16: *static_cast<int16_t*>(p) = static_cast<int16_t>(v); break;
                case asTYPEID_INT64: *static_cast<int64_t*>(p) = v; break;
                default: *static_cast<int32_t*>(p) = static_cast<int32_t>(v); break;
            }
        }

        uint64_t LoadUnsigned(int type_id, const void* p) {
            switch (type_id) {
                case asTYPEID_UINT8: return *static_cast<const uint8_t*>(p);
                case asTYPEID_UINT16: return *static_cast<const uint16_t*>(p);
                case asTYPEID_UINT64: return *static_cast<const uint64_t*>(p);
                default: return *static_cast<const uint32_t*>(p);
            }
        }

        void StoreUnsigned(int type_id, void* p, uint64_t v) {
            switch (type_id) {
                case asTYPEID_UINT8: *static_cast<uint8_t*>(p) = static_cast<uint8_t>(v); break;
                case asTYPEID_UINT16: *static_cast<uint16_t*>(p) = static_cast<uint16_t>(v); break;
                case asTYPEID_UINT64: *static_cast<uint64_t*>(p) = v; break;
                default: *static_cast<uint32_t*>(p) = static_cast<uint32_t>(v); break;
            }
        }

        bool WriteProperty(
            PropKind kind, int type_id, const void* p,
            BinaryWriter& w, const ACCS::SaveContext& ctx) {
            switch (kind) {
                case PropKind::Bool: return w.WriteBool(*static_cast<const bool*>(p));
                case PropKind::Signed: return w.WriteI64(LoadSigned(type_id, p));
                case PropKind::Unsigned: return w.WriteU64(LoadUnsigned(type_id, p));
                case PropKind::Float: return w.WriteF32(*static_cast<const float*>(p));
                case PropKind::Double: return w.WriteF64(*static_cast<const double*>(p));
                case PropKind::String: return w.WriteSizedString(*static_cast<const std::string*>(p));
                case PropKind::Actor:
                    return w.WriteU32(ctx.Resolve(*static_cast<const ActorHandle*>(p)));
            }
            return false;
        }

        struct PropValue {
            PropKind kind = PropKind::Bool;
            bool b = false;
            int64_t i = 0;
            uint64_t u = 0;
            double d = 0.0;
            std::string s;
            ActorHandle actor;
        };

        // Always consumes the value's bytes so unmatched properties can be
        // skipped. An unknown kind can't be skipped, so it fails the chunk.
        bool ReadValue(PropKind kind, BinaryReader& r, const ACCS::LoadContext& ctx, PropValue& v) {
            v.kind = kind;
            switch (kind) {
                case PropKind::Bool: return r.ReadBool(v.b);
                case PropKind::Signed: return r.ReadI64(v.i);
                case PropKind::Unsigned: return r.ReadU64(v.u);
                case PropKind::Float: {
                    float f = 0.0f;
                    if (!r.ReadF32(f)) return false;
                    v.d = f;
                    return true;
                }
                case PropKind::Double: return r.ReadF64(v.d);
                case PropKind::String: return r.ReadSizedString(v.s);
                case PropKind::Actor: {
                    uint32_t ref = 0;
                    if (!r.ReadU32(ref)) return false;
                    v.actor = ctx.Resolve(ref);
                    return true;
                }
            }
            return false;
        }

        // Returns false (and leaves the property untouched) on a kind mismatch.
        bool ApplyValue(PropKind target, int type_id, void* p, const PropValue& v) {
            switch (target) {
                case PropKind::Bool:
                    if (v.kind != PropKind::Bool) return false;
                    *static_cast<bool*>(p) = v.b;
                    return true;
                case PropKind::Signed:
                    if (v.kind != PropKind::Signed) return false;
                    StoreSigned(type_id, p, v.i);
                    return true;
                case PropKind::Unsigned:
                    if (v.kind != PropKind::Unsigned) return false;
                    StoreUnsigned(type_id, p, v.u);
                    return true;
                case PropKind::Float:
                    if (v.kind != PropKind::Float && v.kind != PropKind::Double) return false;
                    *static_cast<float*>(p) = static_cast<float>(v.d);
                    return true;
                case PropKind::Double:
                    if (v.kind != PropKind::Float && v.kind != PropKind::Double) return false;
                    *static_cast<double*>(p) = v.d;
                    return true;
                case PropKind::String:
                    if (v.kind != PropKind::String) return false;
                    *static_cast<std::string*>(p) = v.s;
                    return true;
                case PropKind::Actor:
                    if (v.kind != PropKind::Actor) return false;
                    *static_cast<ActorHandle*>(p) = v.actor;
                    return true;
            }
            return false;
        }

        bool SaveScriptObject(
            asIScriptObject& object, BinaryWriter& w,
            const ACCS::SaveContext& ctx, const ScriptTypeIds& ids) {
            asIScriptEngine& engine = *object.GetEngine();

            struct Item {
                asUINT index;
                PropKind kind;
                int type_id;
            };
            std::vector<Item> items;

            for (asUINT i = 0; i < object.GetPropertyCount(); ++i) {
                const int type_id = object.GetPropertyTypeId(i);
                if (auto kind = ClassifyProperty(engine, type_id, ids)) {
                    items.push_back({i, *kind, type_id});
                }
            }

            if (!w.WriteU32(static_cast<uint32_t>(items.size()))) {
                return false;
            }

            for (const Item& item : items) {
                if (!w.WriteSizedString(object.GetPropertyName(item.index)) ||
                    !w.WriteU8(static_cast<uint8_t>(item.kind)) ||
                    !WriteProperty(item.kind, item.type_id,
                                   object.GetAddressOfProperty(item.index), w, ctx)) {
                    return false;
                }
            }
            return true;
        }

        bool LoadScriptObject(
            asIScriptObject& object, BinaryReader& r,
            const ACCS::LoadContext& ctx, const ScriptTypeIds& ids) {
            asIScriptEngine& engine = *object.GetEngine();

            std::unordered_map<std::string, asUINT> by_name;
            for (asUINT i = 0; i < object.GetPropertyCount(); ++i) {
                by_name.emplace(object.GetPropertyName(i), i);
            }

            uint32_t count = 0;
            if (!r.ReadU32(count)) {
                return false;
            }

            for (uint32_t n = 0; n < count; ++n) {
                std::string name;
                uint8_t raw_kind = 0;
                PropValue value;

                if (!r.ReadSizedString(name) || !r.ReadU8(raw_kind) ||
                    !ReadValue(static_cast<PropKind>(raw_kind), r, ctx, value)) {
                    return false;
                }

                const auto it = by_name.find(name);
                if (it == by_name.end()) {
                    continue; // property no longer exists in the script
                }

                const int type_id = object.GetPropertyTypeId(it->second);
                const auto target = ClassifyProperty(engine, type_id, ids);
                if (!target) {
                    continue;
                }

                ApplyValue(*target, type_id, object.GetAddressOfProperty(it->second), value);
            }
            return true;
        }

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

        ACCS::World& GetWorld(Runtime& runtime) {
            return GetACCS(runtime).world;
        }

        template <typename T>
        CScriptArray* MakeArray(
            Runtime& runtime,
            std::string_view declaration,
            const std::vector<T>& values) {
            const std::string type_name = std::string(declaration);
            asITypeInfo* type = runtime.GetScriptEngine().GetTypeInfoByDecl(type_name.c_str());
            if (!type) {
                return nullptr;
            }

            CScriptArray* array = CScriptArray::Create(type, static_cast<asUINT>(values.size()));
            for (asUINT index = 0; index < values.size(); ++index) {
                *static_cast<T*>(array->At(index)) = values[index];
            }
            return array;
        }

        ActorHandle CreateActor(Runtime& runtime) {
            return GetWorld(runtime).CreateActor();
        }

        bool DestroyActor(const ActorHandle& actor, Runtime& runtime) {
            return GetWorld(runtime).DestroyActor(actor);
        }

        bool IsActorAlive(const ActorHandle& actor, Runtime& runtime) {
            return GetWorld(runtime).IsActorAlive(actor);
        }

        CScriptArray* GetActors(Runtime& runtime) {
            return MakeArray(runtime, "array<CE::ACCS::ActorHandle>", GetWorld(runtime).GetActors());
        }

        bool SetActorName(const ActorHandle& actor, const std::string& name, Runtime& runtime) {
            return GetWorld(runtime).SetActorName(actor, name);
        }

        std::string GetActorName(const ActorHandle& actor, Runtime& runtime) {
            const std::string* name = GetWorld(runtime).GetActorName(actor);
            return name ? *name : std::string();
        }

        bool SetActorActive(const ActorHandle& actor, bool active, Runtime& runtime) {
            return GetWorld(runtime).SetActorActive(actor, active);
        }

        bool IsActorActive(const ActorHandle& actor, Runtime& runtime) {
            return GetWorld(runtime).IsActorActive(actor);
        }

        bool AddActorTag(const ActorHandle& actor, const std::string& tag, Runtime& runtime) {
            return GetWorld(runtime).AddActorTag(actor, tag);
        }

        bool RemoveActorTag(const ActorHandle& actor, const std::string& tag, Runtime& runtime) {
            return GetWorld(runtime).RemoveActorTag(actor, tag);
        }

        bool HasActorTag(const ActorHandle& actor, const std::string& tag, Runtime& runtime) {
            return GetWorld(runtime).HasActorTag(actor, tag);
        }

        CScriptArray* GetActorTags(const ActorHandle& actor, Runtime& runtime) {
            static const std::vector<std::string> kNoTags;
            const std::vector<std::string>* tags = GetWorld(runtime).GetActorTags(actor);
            return MakeArray(runtime, "array<string>", tags ? *tags : kNoTags);
        }

        ComponentID AddComponentByName(const ActorHandle& actor, const std::string& name, Runtime& runtime) {
            const ComponentID id = GetWorld(runtime).AddComponent(actor, name);
            if (!id.IsValid()) {
                CE_LOG(LogLevel::Error,
                       "[ACCS] AddComponent('{}') failed: unknown component, invalid actor, or duplicate",
                       name);
            }
            return id;
        }

        // Adds an already-constructed script object, so scripts can do:
        //   Health h; h.hp = 5; AddComponent(actor, h);
        ComponentID AddComponentObject(const ActorHandle& actor, asIScriptObject* object, Runtime& runtime) {
            if (!object) {
                return {};
            }

            ACCS::World& world = GetWorld(runtime);
            const ACCS::ComponentTypeInfo* info = world.GetRegistry().Find(
                static_cast<ACCS::ComponentTypeKey>(object->GetObjectType()));

            if (!info) {
                CE_LOG(LogLevel::Error,
                       "[ACCS] Script class '{}' is not a registered component; call RegisterComponent first",
                       object->GetObjectType()->GetName());
                return {};
            }

            try {
                return world.AddComponentInstance(
                    actor, info->name, std::make_shared<ScriptComponent>(*object));
            } catch (const std::exception& exception) {
                CE_LOG(LogLevel::Error, "[ACCS] Failed to add component '{}': {}", info->name, exception.what());
                return {};
            }
        }

        bool HasComponentByName(const ActorHandle& actor, const std::string& name, Runtime& runtime) {
            return GetWorld(runtime).HasComponent(actor, name);
        }

        bool HasComponentByID(const ComponentID& component, Runtime& runtime) {
            return GetWorld(runtime).HasComponent(component);
        }

        bool RemoveComponentByID(const ComponentID& component, Runtime& runtime) {
            return GetWorld(runtime).RemoveComponent(component);
        }

        bool RemoveComponentByName(const ActorHandle& actor, const std::string& name, Runtime& runtime) {
            ACCS::World& world = GetWorld(runtime);
            for (const ComponentID id : world.GetComponents(actor)) {
                const std::string* component_name = world.GetComponentName(id);
                if (component_name && *component_name == name) {
                    return world.RemoveComponent(id);
                }
            }
            return false;
        }

        std::string GetComponentName(const ComponentID& component, Runtime& runtime) {
            const std::string* name = GetWorld(runtime).GetComponentName(component);
            return name ? *name : std::string();
        }

        CScriptArray* GetComponents(const ActorHandle& actor, Runtime& runtime) {
            return MakeArray(runtime, "array<CE::ACCS::ComponentID>", GetWorld(runtime).GetComponents(actor));
        }

        // Script components retain their concrete type; native components are
        // intentionally not exposed as IComponent handles (use Get<Name>).
        asIScriptObject* ToScriptObject(ACCS::IComponent* component) {
            auto* script_component = dynamic_cast<ScriptComponent*>(component);
            if (!script_component) {
                return nullptr;
            }
            asIScriptObject& object = script_component->GetObject();
            object.AddRef();
            return &object;
        }

        asIScriptObject* GetComponentObjectByID(const ComponentID& component, Runtime& runtime) {
            return ToScriptObject(GetWorld(runtime).GetComponent(component));
        }

        asIScriptObject* GetComponentObjectByName(const ActorHandle& actor, const std::string& name, Runtime& runtime) {
            return ToScriptObject(GetWorld(runtime).GetComponent(actor, name));
        }

        // Declares a script class as a component:
        //   RegisterComponent("Health");            // name == class name
        //   RegisterComponent("Health", "MyNs::HealthComponent");
        // Must be called from script, because the class is looked up in the
        // module of the calling function.
        bool RegisterScriptComponent(const std::string& name, const std::string& class_name, Runtime& runtime) {
            asIScriptContext* context = asGetActiveContext();
            asIScriptFunction* caller = context ? context->GetFunction() : nullptr;
            asIScriptModule* module = caller ? caller->GetModule() : nullptr;
            if (!module) {
                CE_LOG(LogLevel::Error, "[ACCS] RegisterComponent('{}') must be called from script", name);
                return false;
            }

            asITypeInfo* type = module->GetTypeInfoByDecl(class_name.c_str());
            if (!type || !(type->GetFlags() & asOBJ_SCRIPT_OBJECT)) {
                CE_LOG(LogLevel::Error, "[ACCS] RegisterComponent('{}'): script class '{}' not found",
                       name, class_name);
                return false;
            }

            asIScriptEngine& engine = runtime.GetScriptEngine();

            const asITypeInfo* base = engine.GetTypeInfoByDecl("CE::ACCS::IComponent");
            if (base && !type->Implements(base)) {
                CE_LOG(LogLevel::Error, "[ACCS] RegisterComponent('{}'): '{}' does not implement IComponent",
                       name, class_name);
                return false;
            }

            ScriptTypeIds ids;
            ids.string_id = engine.GetTypeIdByDecl("string");
            ids.actor_id = engine.GetTypeIdByDecl("CE::ACCS::ActorHandle");

            ACCS::ComponentTypeInfo info;
            info.name = name;
            info.key = type; // matches asIScriptObject::GetObjectType()

            info.create = [&engine, type]() -> std::shared_ptr<ACCS::IComponent> {
                auto* object = static_cast<asIScriptObject*>(engine.CreateScriptObject(type));
                if (!object) {
                    return nullptr;
                }
                auto component = std::make_shared<ScriptComponent>(*object); // takes its own ref
                object->Release();                                            // drop the creation ref
                return component;
            };

            info.save = [ids](const ACCS::IComponent& c, BinaryWriter& w, const ACCS::SaveContext& ctx) {
                const auto& component = static_cast<const ScriptComponent&>(c);
                return SaveScriptObject(component.GetObject(), w, ctx, ids);
            };

            info.load = [ids](ACCS::IComponent& c, BinaryReader& r, const ACCS::LoadContext& ctx) {
                auto& component = static_cast<ScriptComponent&>(c);
                return LoadScriptObject(component.GetObject(), r, ctx, ids);
            };

            if (!GetWorld(runtime).GetRegistry().Add(std::move(info))) {
                CE_LOG(LogLevel::Error,
                       "[ACCS] RegisterComponent('{}'): name or class already registered", name);
                return false;
            }
            return true;
        }

        bool RegisterScriptComponentSameName(const std::string& class_name, Runtime& runtime) {
            return RegisterScriptComponent(class_name, class_name, runtime);
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
                GetACCS(runtime).system_manager.RegisterSystem(types, std::move(system));
            } catch (const std::exception& exception) {
                CE_LOG(LogLevel::Error, "[AngelScript] Failed to register ACCS system: {}", exception.what());
            }
        }

        template <typename T>
        T* GetNativeByID(const ComponentID& id, Runtime& runtime) {
            // World::GetComponent<T>(id) checks the stored type key, so a wrong ID returns nullptr.
            return GetWorld(runtime).GetComponent<T>(id);
        }

        template <typename T>
        T* GetNativeByActor(const ActorHandle& actor, Runtime& runtime) {
            return GetWorld(runtime).GetComponent<T>(actor);
        }

        // Exposes native component T to scripts as:
        //   - an object type `Name` with the members from bindMembers
        //   - Name@ GetName(ActorHandle) / Name@ GetName(ComponentID)
        //   - AddComponent(actor, "Name") and the other by-name functions
        //     (via the world registry; T is registered there if it isn't yet).
        // Script handles to native components don't keep them alive: don't
        // hold one across a RemoveComponent/DestroyActor.
        template <typename T, typename BindMembers>
        bool RegisterNativeComponent(
            asIScriptEngine& engine,
            Runtime& runtime,
            const std::string& name,
            BindMembers&& bindMembers
        ) {
            ACCS::World& world = GetWorld(runtime);
            const ACCS::ComponentTypeInfo* existing =
                world.GetRegistry().Find(ACCS::GetComponentTypeKey<T>());

            if (!existing) {
                if (!world.RegisterComponent<T>(name)) {
                    CE_LOG(LogLevel::Error, "[ACCS] Failed to register native component '{}'", name);
                    return false;
                }
            } else if (existing->name != name) {
                CE_LOG(LogLevel::Error,
                       "[ACCS] Native component registered as '{}' in C++ but bound as '{}' in scripts",
                       existing->name, name);
                return false;
            }

            CE_CHECK_AS(engine.RegisterObjectType(name.c_str(), 0, asOBJ_REF | asOBJ_NOCOUNT));

            if (!bindMembers(name.c_str())) {
                return false;
            }

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

        CE::Common::FS::VFS::VFS& GetVFS(Runtime& runtime) {
            return runtime.mVFS;
        }

        bool SaveWorld(const std::string& path, Runtime& runtime) {
            const bool ok = ACCS::SaveWorldToFile(GetWorld(runtime), GetVFS(runtime), path);
            if (!ok) {
                CE_LOG(LogLevel::Error, "[ACCS] Failed to save world to '{}'", path);
            }
            return ok;
        }

        ACCS::LoadResult LoadWorld(const std::string& path, Runtime& runtime) {
            const ACCS::LoadResult result = ACCS::LoadWorldFromFile(GetWorld(runtime), GetVFS(runtime), path);
            if (result == ACCS::LoadResult::Failed) {
                CE_LOG(LogLevel::Error, "[ACCS] Failed to load world from '{}'", path);
            } else if (result == ACCS::LoadResult::Partial) {
                CE_LOG(LogLevel::Warn, "[ACCS] World '{}' loaded with dropped components", path);
            }
            return result;
        }
    } // namespace

#define ACCS_BIND(decl, fn) \
    CE_CHECK_AS(mScriptEngine.RegisterGlobalFunction( \
        decl, asFUNCTION(fn), asCALL_CDECL_OBJLAST, &mRuntime))

    bool ACCSBinding::RegisterBindings() {
        NamespaceScope ns(mScriptEngine, "CE::ACCS");

        ACCS_BIND("ActorHandle CreateActor()", CreateActor);
        ACCS_BIND("bool DestroyActor(const ActorHandle &in)", DestroyActor);
        ACCS_BIND("bool IsActorAlive(const ActorHandle &in)", IsActorAlive);
        ACCS_BIND("array<ActorHandle>@ GetActors()", GetActors);

        ACCS_BIND("bool SetActorName(const ActorHandle &in, const string &in)", SetActorName);
        ACCS_BIND("string GetActorName(const ActorHandle &in)", GetActorName);
        ACCS_BIND("bool SetActorActive(const ActorHandle &in, bool)", SetActorActive);
        ACCS_BIND("bool IsActorActive(const ActorHandle &in)", IsActorActive);
        ACCS_BIND("bool AddActorTag(const ActorHandle &in, const string &in)", AddActorTag);
        ACCS_BIND("bool RemoveActorTag(const ActorHandle &in, const string &in)", RemoveActorTag);
        ACCS_BIND("bool HasActorTag(const ActorHandle &in, const string &in)", HasActorTag);
        ACCS_BIND("array<string>@ GetActorTags(const ActorHandle &in)", GetActorTags);

        ACCS_BIND("bool RegisterComponent(const string &in className)", RegisterScriptComponentSameName);
        ACCS_BIND("bool RegisterComponent(const string &in name, const string &in className)", RegisterScriptComponent);

        ACCS_BIND("ComponentID AddComponent(const ActorHandle &in, const string &in name)", AddComponentByName);
        ACCS_BIND("ComponentID AddComponent(const ActorHandle &in, IComponent@ component)", AddComponentObject);

        ACCS_BIND("bool HasComponent(const ActorHandle &in, const string &in name)", HasComponentByName);
        ACCS_BIND("bool HasComponent(const ComponentID &in)", HasComponentByID);

        ACCS_BIND("bool RemoveComponent(const ActorHandle &in, const string &in name)", RemoveComponentByName);
        ACCS_BIND("bool RemoveComponent(const ComponentID &in)", RemoveComponentByID);

        ACCS_BIND("IComponent@ GetComponent(const ActorHandle &in, const string &in name)", GetComponentObjectByName);
        ACCS_BIND("IComponent@ GetComponent(const ComponentID &in)", GetComponentObjectByID);

        ACCS_BIND("string GetComponentName(const ComponentID &in)", GetComponentName);
        ACCS_BIND("array<ComponentID>@ GetComponents(const ActorHandle &in)", GetComponents);

        ACCS_BIND("void RegisterSystem(FrameUpdateType, ISystem@)", RegisterSystem);
        ACCS_BIND("bool SaveWorld(const string &in path)", SaveWorld);
        ACCS_BIND("LoadResult LoadWorld(const string &in path)", LoadWorld);

        return RegisterNativeComponent<ACCS::Components::Transform>(
                mScriptEngine, mRuntime, "Transform",
                [&](const char* n) {
                    CE_CHECK_AS(mScriptEngine.RegisterObjectProperty(
                        n, "Transform3D transform_3d",
                        asOFFSET(ACCS::Components::Transform, transform_3d)));
                    CE_CHECK_AS(mScriptEngine.RegisterObjectProperty(
                        n, "Transform2D transform_2d",
                        asOFFSET(ACCS::Components::Transform, transform_2d)));
                    return true;
                });
    }

#undef ACCS_BIND
} // namespace CE::Scripting::Bindings