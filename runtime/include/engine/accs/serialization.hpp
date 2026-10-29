#pragma once

#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ce_common/binary_reader.hpp"
#include "ce_common/binary_writer.hpp"
#include "engine/accs/serialization_context.hpp"
#include "engine/accs/world.hpp"

namespace CE::ACCS {
    enum class LoadResult {
        Ok,       // everything loaded (unknown/transient types are skipped silently)
        Partial,  // some components failed to load and were dropped
        Failed    // corrupt/unsupported file; nothing was added to the world
    };

    class WorldSerializer {
        public:
            static constexpr uint32_t kMaxTags = 1u << 16;
            static constexpr uint32_t kFormatVersion = 1;
            static constexpr uint32_t kMaxChunkSize  = 256u * 1024 * 1024;
            static constexpr uint32_t kMaxActors     = 1u << 24;
            static constexpr uint32_t kMaxTypes      = 1u << 16;

            // Open the stream with std::ios::binary. Returns false on any failure.
            static bool Save(const World& world, std::ostream& os) {
                BinaryWriter w(os);
                const std::vector<ActorHandle> actors = world.GetActors();

                SaveContext ctx;
                for (uint32_t i = 0; i < actors.size(); ++i) {
                    ctx.mIds[SaveContext::Key(actors[i])] = i;
                }

                struct Entry {
                    uint32_t type;
                    const ComponentTypeInfo* info;
                    ComponentID id;
                };

                std::vector<std::string> typeNames;
                std::unordered_map<std::string, uint32_t> typeIndex;
                std::vector<std::vector<Entry>> perActor(actors.size());

                for (size_t i = 0; i < actors.size(); ++i) {
                    for (ComponentID cid : world.GetComponents(actors[i])) {
                        const auto& entries = perActor[i];

                        std::ostringstream mbuf(std::ios::binary);
                        BinaryWriter mw(mbuf);
                        if (!WriteActorMeta(world, actors[i], mw) || !mbuf) return false;

                        const std::string meta = mbuf.str();
                        if (meta.size() > kMaxChunkSize ||
                            !w.WriteU32(static_cast<uint32_t>(meta.size())) ||
                            !w.WriteBytes(meta.data(), meta.size())) {
                            return false;
                        }

                        if (!w.WriteU32(static_cast<uint32_t>(entries.size()))) return false;

                        const std::string* name = world.GetComponentName(cid);
                        const ComponentTypeInfo* info =
                            name ? world.GetRegistry().Find(*name) : nullptr;

                        if (!info || !info->IsSerializable()) {
                            continue; // transient or unregistered
                        }

                        auto [it, inserted] = typeIndex.try_emplace(
                            *name, static_cast<uint32_t>(typeNames.size()));
                        if (inserted) {
                            typeNames.push_back(*name);
                        }

                        perActor[i].push_back({it->second, info, cid});
                    }
                }

                if (!w.WriteBytes("CEWD", 4) || !w.WriteU32(kFormatVersion)) {
                    return false;
                }

                if (!w.WriteU32(static_cast<uint32_t>(typeNames.size()))) {
                    return false;
                }
                for (const std::string& n : typeNames) {
                    if (!w.WriteSizedString(n)) {
                        return false;
                    }
                }

                if (!w.WriteU32(static_cast<uint32_t>(actors.size()))) {
                    return false;
                }

                for (const auto& entries : perActor) {
                    if (!w.WriteU32(static_cast<uint32_t>(entries.size()))) {
                        return false;
                    }

                    for (const Entry& e : entries) {
                        const IComponent* comp = world.GetComponent(e.id);
                        if (!comp) {
                            return false;
                        }

                        // Buffer the chunk so its size is known up front and
                        // the target stream doesn't need to be seekable.
                        std::ostringstream buf(std::ios::binary);
                        BinaryWriter cw(buf);
                        if (!e.info->save(*comp, cw, ctx) || !buf) {
                            return false;
                        }

                        const std::string bytes = buf.str();
                        if (bytes.size() > kMaxChunkSize) {
                            return false;
                        }

                        if (!w.WriteU32(e.type) ||
                            !w.WriteU32(static_cast<uint32_t>(bytes.size())) ||
                            !w.WriteBytes(bytes.data(), bytes.size())) {
                            return false;
                        }
                    }
                }

                return w.Flush();
            }

            // Appends to `world` (so it also works for prefabs/additive loads).
            // Component types must be registered before calling this.
            static LoadResult Load(World& world, std::istream& is) {
                BinaryReader r(is);
                LoadContext ctx;

                // On a structural failure, remove everything we created.
                auto fail = [&]() {
                    for (ActorHandle h : ctx.mHandles) {
                        world.DestroyActor(h);
                    }
                    return LoadResult::Failed;
                };

                char magic[4];
                uint32_t version = 0;
                uint32_t typeCount = 0;
                uint32_t actorCount = 0;

                if (!r.Read(magic, 4) || std::memcmp(magic, "CEWD", 4) != 0) {
                    return fail();
                }
                if (!r.ReadU32(version) || version != kFormatVersion) {
                    return fail();
                }

                if (!r.ReadU32(typeCount) || typeCount > kMaxTypes) {
                    return fail();
                }

                std::vector<const ComponentTypeInfo*> types(typeCount);
                for (auto& t : types) {
                    std::string name;
                    if (!r.ReadSizedString(name)) {
                        return fail();
                    }
                    t = world.GetRegistry().Find(name); // null = unknown, chunks skipped
                }

                if (!r.ReadU32(actorCount) || actorCount > kMaxActors) {
                    return fail();
                }

                // Create every actor first so references can point forward.
                ctx.mHandles.reserve(actorCount);
                for (uint32_t i = 0; i < actorCount; ++i) {
                    ctx.mHandles.push_back(world.CreateActor());
                }

                bool partial = false;
                
                for (uint32_t i = 0; i < actorCount; ++i) {
                    uint32_t metaSize = 0;
                    if (!r.ReadU32(metaSize) || metaSize > kMaxChunkSize) return fail();

                    std::string meta(metaSize, '\0');
                    if (metaSize > 0 && !r.Read(meta.data(), metaSize)) return fail();

                    {
                        std::istringstream mbuf(meta, std::ios::binary);
                        BinaryReader mr(mbuf);
                        if (!ReadActorMeta(world, ctx.mHandles[i], mr)) partial = true; // keep defaults
                    }
                    
                    uint32_t count = 0;
                    if (!r.ReadU32(count)) {
                        return fail();
                    }

                    for (uint32_t c = 0; c < count; ++c) {
                        uint32_t typeIdx = 0;
                        uint32_t size = 0;

                        if (!r.ReadU32(typeIdx) || !r.ReadU32(size)) {
                            return fail();
                        }
                        if (typeIdx >= typeCount || size > kMaxChunkSize) {
                            return fail();
                        }

                        std::string bytes(size, '\0');
                        if (size > 0 && !r.Read(bytes.data(), size)) {
                            return fail();
                        }

                        const ComponentTypeInfo* info = types[typeIdx];
                        if (!info || !info->IsSerializable()) {
                            continue; // unknown or transient type: skip chunk
                        }

                        ComponentID id = world.AddComponent(ctx.mHandles[i], info->name);
                        IComponent* comp = world.GetComponent(id);
                        if (!comp) {
                            partial = true;
                            continue;
                        }

                        std::istringstream buf(bytes, std::ios::binary);
                        BinaryReader cr(buf);
                        if (!info->load(*comp, cr, ctx)) {
                            world.RemoveComponent(id);
                            partial = true;
                        }
                    }
                }

                return partial ? LoadResult::Partial : LoadResult::Ok;
            }
        private:
            static bool WriteActorMeta(const World& world, ActorHandle h, BinaryWriter& w) {
                const std::string* name = world.GetActorName(h);
                const std::vector<std::string>* tags = world.GetActorTags(h);
                if (!name || !tags) return false;

                if (!w.WriteSizedString(*name) ||
                    !w.WriteBool(world.IsActorActive(h)) ||
                    !w.WriteU32(static_cast<uint32_t>(tags->size()))) {
                    return false;
                }
                for (const std::string& t : *tags) {
                    if (!w.WriteSizedString(t)) return false;
                }
                return true;
            }

            // Parses fully before applying, so a bad chunk leaves the actor untouched.
            static bool ReadActorMeta(World& world, ActorHandle h, BinaryReader& r) {
                std::string name;
                bool active = true;
                uint32_t tagCount = 0;

                if (!r.ReadSizedString(name) || !r.ReadBool(active) ||
                    !r.ReadU32(tagCount) || tagCount > kMaxTags) {
                    return false;
                }

                std::vector<std::string> tags(tagCount);
                for (std::string& t : tags) {
                    if (!r.ReadSizedString(t)) return false;
                }

                // Fields appended in future versions go here, guarded by r.AtEnd().

                world.SetActorName(h, std::move(name));
                world.SetActorActive(h, active);
                for (std::string& t : tags) world.AddActorTag(h, std::move(t));
                return true;
            }
    };
}
