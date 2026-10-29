#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "ce_common/binary_reader.hpp"
#include "ce_common/binary_writer.hpp"
#include "engine/accs/component.hpp"

namespace CE::ACCS {
    using CECommon::FS::BinaryReader;
    using CECommon::FS::BinaryWriter;

    class SaveContext;
    class LoadContext;

    struct ComponentTypeInfo {
        std::string name;
        ComponentTypeKey key = nullptr;
        std::function<std::shared_ptr<IComponent>()> create;

        // Empty = transient (never written to disk).
        std::function<bool(const IComponent&, BinaryWriter&, const SaveContext&)> save;
        std::function<bool(IComponent&, BinaryReader&, const LoadContext&)> load;

        bool IsSerializable() const { return save && load; }
    };

    namespace detail {
        // Detects: bool Serialize(BinaryWriter&, const SaveContext&) const
        //          bool Deserialize(BinaryReader&, const LoadContext&)
        template <typename T, typename = void>
        struct HasSerialization : std::false_type {};

        template <typename T>
        struct HasSerialization<T, std::void_t<
            decltype(static_cast<bool>(std::declval<const T&>().Serialize(
                std::declval<BinaryWriter&>(), std::declval<const SaveContext&>()))),
            decltype(static_cast<bool>(std::declval<T&>().Deserialize(
                std::declval<BinaryReader&>(), std::declval<const LoadContext&>())))>>
            : std::true_type {};
    }

    class ComponentRegistry {
        public:
            // Returns false if the name is empty, or the name or type is
            // already registered.
            template <typename T>
            bool Register(std::string name) {
                static_assert(std::is_base_of_v<IComponent, T>,
                    "T must inherit from IComponent");
                static_assert(std::is_default_constructible_v<T>,
                    "Registered components must be default constructible");

                ComponentTypeInfo info;
                info.name = std::move(name);
                info.key = GetComponentTypeKey<T>();
                info.create = [] { return std::make_shared<T>(); };

                if constexpr (detail::HasSerialization<T>::value) {
                    info.save = [](const IComponent& c, BinaryWriter& w, const SaveContext& ctx) {
                        return static_cast<bool>(static_cast<const T&>(c).Serialize(w, ctx));
                    };
                    info.load = [](IComponent& c, BinaryReader& r, const LoadContext& ctx) {
                        return static_cast<bool>(static_cast<T&>(c).Deserialize(r, ctx));
                    };
                }

                return Add(std::move(info));
            }

            // Also used directly for script-defined components with a custom key.
            bool Add(ComponentTypeInfo info) {
                if (info.name.empty() || !info.key || !info.create) {
                    return false;
                }
                if (mByName.count(info.name) || mByKey.count(info.key)) {
                    return false;
                }

                mByKey.emplace(info.key, info.name);
                mByName.emplace(info.name, std::move(info));
                return true;
            }

            const ComponentTypeInfo* Find(const std::string& name) const {
                auto it = mByName.find(name);
                return it == mByName.end() ? nullptr : &it->second;
            }

            const ComponentTypeInfo* Find(ComponentTypeKey key) const {
                auto it = mByKey.find(key);
                return it == mByKey.end() ? nullptr : Find(it->second);
            }

        private:
            std::unordered_map<std::string, ComponentTypeInfo> mByName;
            std::unordered_map<ComponentTypeKey, std::string> mByKey;
    };
}