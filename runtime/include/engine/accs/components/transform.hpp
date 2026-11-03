#pragma once

#include "ce_common/binary_reader.hpp"
#include "ce_common/binary_writer.hpp"
#include "engine/accs/component.hpp"
#include "engine/accs/serialization_context.hpp"
#include "engine/rendering/renderer.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::ACCS::Components {
    class Transform : public IComponent {
        public:
            Renderer::Transform3D transform_3d{};
            Renderer::Transform2D transform_2d{};

            bool Serialize(CECommon::FS::BinaryWriter& writer, const SaveContext&) const {
                CE_LOG(
                    LogLevel::Debug,
                    "Transform Serialize: position=({}, {}, {})",
                    transform_3d.position.x,
                    transform_3d.position.y,
                    transform_3d.position.z
                );

                CE_LOG(
                    LogLevel::Debug,
                    "Transform Serialize: rotation=({}, {}, {})",
                    transform_3d.rotation.x,
                    transform_3d.rotation.y,
                    transform_3d.rotation.z
                );

                CE_LOG(
                    LogLevel::Debug,
                    "Transform Serialize: scale=({}, {}, {})",
                    transform_3d.scale.x,
                    transform_3d.scale.y,
                    transform_3d.scale.z
                );

                return WriteVec3(writer, transform_3d.position) &&
                    WriteVec3(writer, transform_3d.rotation) &&
                    WriteVec3(writer, transform_3d.scale) &&
                    WriteVec2(writer, transform_2d.position) &&
                    writer.WriteF32(transform_2d.rotation) &&
                    WriteVec2(writer, transform_2d.scale);
            }

            bool Deserialize(CECommon::FS::BinaryReader& reader, const LoadContext&) {
                Renderer::Transform3D transform3d{};
                Renderer::Transform2D transform2d{};

                if (!ReadVec3(reader, transform3d.position) ||
                    !ReadVec3(reader, transform3d.rotation) ||
                    !ReadVec3(reader, transform3d.scale) ||
                    !ReadVec2(reader, transform2d.position) ||
                    !reader.ReadF32(transform2d.rotation) ||
                    !ReadVec2(reader, transform2d.scale)) {
                    return false;
                }

                transform_3d = transform3d;
                transform_2d = transform2d;

                CE_LOG(
                    LogLevel::Debug,
                    "Transform Deserialize: position=({}, {}, {})",
                    transform3d.position.x,
                    transform3d.position.y,
                    transform3d.position.z
                );

                CE_LOG(
                    LogLevel::Debug,
                    "Transform Deserialize: rotation=({}, {}, {})",
                    transform3d.rotation.x,
                    transform3d.rotation.y,
                    transform3d.rotation.z
                );

                CE_LOG(
                    LogLevel::Debug,
                    "Transform Deserialize: scale=({}, {}, {})",
                    transform3d.scale.x,
                    transform3d.scale.y,
                    transform3d.scale.z
                );
        
                return true;
            }

        private:
            static bool WriteVec2(CECommon::FS::BinaryWriter& writer, const glm::vec2& value) {
                return writer.WriteF32(value.x) && writer.WriteF32(value.y);
            }

            static bool WriteVec3(CECommon::FS::BinaryWriter& writer, const glm::vec3& value) {
                return writer.WriteF32(value.x) && writer.WriteF32(value.y) &&
                    writer.WriteF32(value.z);
            }

            static bool ReadVec2(CECommon::FS::BinaryReader& reader, glm::vec2& value) {
                return reader.ReadF32(value.x) && reader.ReadF32(value.y);
            }

            static bool ReadVec3(CECommon::FS::BinaryReader& reader, glm::vec3& value) {
                return reader.ReadF32(value.x) && reader.ReadF32(value.y) &&
                    reader.ReadF32(value.z);
            }
    };
}
