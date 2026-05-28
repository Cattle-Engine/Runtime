#include "engine/rendering/common/primitives_3d.hpp"

#include <cmath>
#include <cstdint>
#include <numbers>

namespace CE::Renderer::Primitives3D {
    namespace {
        constexpr CE::Renderer::Colour kWhite {255, 255, 255, 255};
        constexpr float kPi = std::numbers::pi_v<float>;

        struct MeshBuilder {
            MeshData data;

            uint32_t AddVertex(
                const glm::vec3& position,
                const glm::vec3& normal,
                const glm::vec2& uv
            ) {
                data.vertices.push_back(Vertex3D{
                    position,
                    normal,
                    kWhite,
                    uv
                });
                return static_cast<uint32_t>(data.vertices.size() - 1);
            }

            void AddQuad(
                const glm::vec3& a,
                const glm::vec3& b,
                const glm::vec3& c,
                const glm::vec3& d,
                const glm::vec3& normal
            ) {
                const uint32_t start = static_cast<uint32_t>(data.vertices.size());
                AddVertex(a, normal, {0.0f, 0.0f});
                AddVertex(b, normal, {1.0f, 0.0f});
                AddVertex(c, normal, {1.0f, 1.0f});
                AddVertex(d, normal, {0.0f, 1.0f});
                data.indices.insert(data.indices.end(), {
                    start + 0, start + 1, start + 2,
                    start + 2, start + 3, start + 0
                });
            }
        };

        inline glm::vec3 SpherePoint(float radius, float theta, float phi) {
            return {
                radius * std::sin(phi) * std::cos(theta),
                radius * std::cos(phi),
                radius * std::sin(phi) * std::sin(theta)
            };
        }

        void ApplyColour(MeshData& data, const Colour& colour) {
            for (auto& vertex : data.vertices) {
                vertex.color = colour;
            }
        }
    }

    void SetMeshColour(MeshData& mesh, const Colour& colour) {
        ApplyColour(mesh, colour);
    }

    MeshData CreateCube(glm::vec3 size) {
        return CreateCube(size, kWhite);
    }

    MeshData CreateCube(glm::vec3 size, const Colour& colour) {
        MeshBuilder builder;
        const glm::vec3 half = size * 0.5f;

        builder.AddQuad(
            {-half.x, -half.y,  half.z},
            { half.x, -half.y,  half.z},
            { half.x,  half.y,  half.z},
            {-half.x,  half.y,  half.z},
            {0.0f, 0.0f, 1.0f}
        );
        builder.AddQuad(
            { half.x, -half.y, -half.z},
            {-half.x, -half.y, -half.z},
            {-half.x,  half.y, -half.z},
            { half.x,  half.y, -half.z},
            {0.0f, 0.0f, -1.0f}
        );
        builder.AddQuad(
            {-half.x, -half.y, -half.z},
            {-half.x, -half.y,  half.z},
            {-half.x,  half.y,  half.z},
            {-half.x,  half.y, -half.z},
            {-1.0f, 0.0f, 0.0f}
        );
        builder.AddQuad(
            { half.x, -half.y,  half.z},
            { half.x, -half.y, -half.z},
            { half.x,  half.y, -half.z},
            { half.x,  half.y,  half.z},
            {1.0f, 0.0f, 0.0f}
        );
        builder.AddQuad(
            {-half.x,  half.y,  half.z},
            { half.x,  half.y,  half.z},
            { half.x,  half.y, -half.z},
            {-half.x,  half.y, -half.z},
            {0.0f, 1.0f, 0.0f}
        );
        builder.AddQuad(
            {-half.x, -half.y, -half.z},
            { half.x, -half.y, -half.z},
            { half.x, -half.y,  half.z},
            {-half.x, -half.y,  half.z},
            {0.0f, -1.0f, 0.0f}
        );

        builder.data.vertex_count = static_cast<uint32_t>(builder.data.vertices.size());
        builder.data.indice_count = static_cast<uint32_t>(builder.data.indices.size());
        ApplyColour(builder.data, colour);
        return builder.data;
    }

    MeshData CreatePlane(glm::vec2 size) {
        return CreatePlane(size, kWhite);
    }

    MeshData CreatePlane(glm::vec2 size, const Colour& colour) {
        MeshBuilder builder;
        const glm::vec2 half = size * 0.5f;

        builder.AddQuad(
            {-half.x, 0.0f, -half.y},
            { half.x, 0.0f, -half.y},
            { half.x, 0.0f,  half.y},
            {-half.x, 0.0f,  half.y},
            {0.0f, 1.0f, 0.0f}
        );

        builder.data.vertex_count = static_cast<uint32_t>(builder.data.vertices.size());
        builder.data.indice_count = static_cast<uint32_t>(builder.data.indices.size());
        ApplyColour(builder.data, colour);
        return builder.data;
    }

    MeshData CreateSphere(float radius, int segments, int rings) {
        return CreateSphere(radius, segments, rings, kWhite);
    }

    MeshData CreateSphere(float radius, int segments, int rings, const Colour& colour) {
        MeshData data;
        segments = std::max(3, segments);
        rings = std::max(2, rings);

        for (int ring = 0; ring <= rings; ++ring) {
            const float v = static_cast<float>(ring) / static_cast<float>(rings);
            const float phi = v * kPi;
            for (int segment = 0; segment <= segments; ++segment) {
                const float u = static_cast<float>(segment) / static_cast<float>(segments);
                const float theta = u * kPi * 2.0f;
                const glm::vec3 position = SpherePoint(radius, theta, phi);
                const glm::vec3 normal = glm::normalize(position);
                data.vertices.push_back({position, normal, colour, {u, v}});
            }
        }

        const int stride = segments + 1;
        for (int ring = 0; ring < rings; ++ring) {
            for (int segment = 0; segment < segments; ++segment) {
                const uint32_t a = static_cast<uint32_t>(ring * stride + segment);
                const uint32_t b = static_cast<uint32_t>((ring + 1) * stride + segment);
                const uint32_t c = static_cast<uint32_t>((ring + 1) * stride + segment + 1);
                const uint32_t d = static_cast<uint32_t>(ring * stride + segment + 1);
                data.indices.insert(data.indices.end(), {a, b, c, c, d, a});
            }
        }

        data.vertex_count = static_cast<uint32_t>(data.vertices.size());
        data.indice_count = static_cast<uint32_t>(data.indices.size());
        return data;
    }

    MeshData CreateCylinder(float radius, float height, int segments) {
        return CreateCylinder(radius, height, segments, kWhite);
    }

    MeshData CreateCylinder(float radius, float height, int segments, const Colour& colour) {
        MeshData data;
        segments = std::max(3, segments);
        const float halfHeight = height * 0.5f;

        const uint32_t topCenter = 0;
        data.vertices.push_back({{0.0f, halfHeight, 0.0f}, {0.0f, 1.0f, 0.0f}, colour, {0.5f, 0.5f}});
        const uint32_t bottomCenter = 1;
        data.vertices.push_back({{0.0f, -halfHeight, 0.0f}, {0.0f, -1.0f, 0.0f}, colour, {0.5f, 0.5f}});

        const uint32_t topRingStart = static_cast<uint32_t>(data.vertices.size());
        for (int segment = 0; segment <= segments; ++segment) {
            const float u = static_cast<float>(segment) / static_cast<float>(segments);
            const float angle = u * kPi * 2.0f;
            const float x = std::cos(angle) * radius;
            const float z = std::sin(angle) * radius;
            data.vertices.push_back({{x, halfHeight, z}, {0.0f, 1.0f, 0.0f}, colour, {u, 0.0f}});
        }

        const uint32_t bottomRingStart = static_cast<uint32_t>(data.vertices.size());
        for (int segment = 0; segment <= segments; ++segment) {
            const float u = static_cast<float>(segment) / static_cast<float>(segments);
            const float angle = u * kPi * 2.0f;
            const float x = std::cos(angle) * radius;
            const float z = std::sin(angle) * radius;
            data.vertices.push_back({{x, -halfHeight, z}, {0.0f, -1.0f, 0.0f}, colour, {u, 1.0f}});
        }

        const uint32_t sideStart = static_cast<uint32_t>(data.vertices.size());
        for (int segment = 0; segment <= segments; ++segment) {
            const float u = static_cast<float>(segment) / static_cast<float>(segments);
            const float angle = u * kPi * 2.0f;
            const float x = std::cos(angle) * radius;
            const float z = std::sin(angle) * radius;
            const glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));
            data.vertices.push_back({{x, halfHeight, z}, normal, kWhite, {u, 0.0f}});
            data.vertices.push_back({{x, -halfHeight, z}, normal, kWhite, {u, 1.0f}});
        }

        for (int segment = 0; segment < segments; ++segment) {
            const uint32_t topA = topRingStart + segment;
            const uint32_t topB = topRingStart + segment + 1;
            data.indices.insert(data.indices.end(), {topCenter, topB, topA});

            const uint32_t bottomA = bottomRingStart + segment;
            const uint32_t bottomB = bottomRingStart + segment + 1;
            data.indices.insert(data.indices.end(), {bottomCenter, bottomA, bottomB});

            const uint32_t sideA = sideStart + static_cast<uint32_t>(segment * 2);
            const uint32_t sideB = sideA + 1;
            const uint32_t sideC = sideA + 3;
            const uint32_t sideD = sideA + 2;
            data.indices.insert(data.indices.end(), {sideA, sideB, sideC, sideC, sideD, sideA});
        }

        data.vertex_count = static_cast<uint32_t>(data.vertices.size());
        data.indice_count = static_cast<uint32_t>(data.indices.size());
        return data;
    }

    MeshData CreateCone(float radius, float height, int segments) {
        return CreateCone(radius, height, segments, kWhite);
    }

    MeshData CreateCone(float radius, float height, int segments, const Colour& colour) {
        MeshData data;
        segments = std::max(3, segments);
        const float halfHeight = height * 0.5f;

        const uint32_t apex = 0;
        data.vertices.push_back({{0.0f, halfHeight, 0.0f}, {0.0f, 1.0f, 0.0f}, colour, {0.5f, 0.0f}});
        const uint32_t center = 1;
        data.vertices.push_back({{0.0f, -halfHeight, 0.0f}, {0.0f, -1.0f, 0.0f}, colour, {0.5f, 0.5f}});

        const uint32_t ringStart = static_cast<uint32_t>(data.vertices.size());
        for (int segment = 0; segment <= segments; ++segment) {
            const float u = static_cast<float>(segment) / static_cast<float>(segments);
            const float angle = u * kPi * 2.0f;
            const float x = std::cos(angle) * radius;
            const float z = std::sin(angle) * radius;
            const glm::vec3 normal = glm::normalize(glm::vec3(x, radius, z));
            data.vertices.push_back({{x, -halfHeight, z}, normal, colour, {u, 1.0f}});
        }

        for (int segment = 0; segment < segments; ++segment) {
            const uint32_t ringA = ringStart + segment;
            const uint32_t ringB = ringStart + segment + 1;
            data.indices.insert(data.indices.end(), {apex, ringA, ringB});
            data.indices.insert(data.indices.end(), {center, ringB, ringA});
        }

        data.vertex_count = static_cast<uint32_t>(data.vertices.size());
        data.indice_count = static_cast<uint32_t>(data.indices.size());
        return data;
    }

    MeshData CreateTorus(float radius, float tubeRadius, int segments, int tubeSegments) {
        return CreateTorus(radius, tubeRadius, segments, tubeSegments, kWhite);
    }

    MeshData CreateTorus(float radius, float tubeRadius, int segments, int tubeSegments, const Colour& colour) {
        MeshData data;
        segments = std::max(3, segments);
        tubeSegments = std::max(3, tubeSegments);

        for (int segment = 0; segment <= segments; ++segment) {
            const float u = static_cast<float>(segment) / static_cast<float>(segments);
            const float theta = u * kPi * 2.0f;
            const glm::vec3 ringCenter {
                std::cos(theta) * radius,
                0.0f,
                std::sin(theta) * radius
            };
            const glm::vec3 ringRight {
                -std::sin(theta),
                0.0f,
                std::cos(theta)
            };
            const glm::vec3 ringUp {0.0f, 1.0f, 0.0f};

            for (int tube = 0; tube <= tubeSegments; ++tube) {
                const float v = static_cast<float>(tube) / static_cast<float>(tubeSegments);
                const float phi = v * kPi * 2.0f;
                const float x = std::cos(phi) * tubeRadius;
                const float y = std::sin(phi) * tubeRadius;
                const glm::vec3 normal = glm::normalize(ringRight * x + ringUp * y);
                const glm::vec3 position = ringCenter + normal * tubeRadius;
                data.vertices.push_back({position, normal, colour, {u, v}});
            }
        }

        const int stride = tubeSegments + 1;
        for (int segment = 0; segment < segments; ++segment) {
            for (int tube = 0; tube < tubeSegments; ++tube) {
                const uint32_t a = static_cast<uint32_t>(segment * stride + tube);
                const uint32_t b = static_cast<uint32_t>((segment + 1) * stride + tube);
                const uint32_t c = static_cast<uint32_t>((segment + 1) * stride + tube + 1);
                const uint32_t d = static_cast<uint32_t>(segment * stride + tube + 1);
                data.indices.insert(data.indices.end(), {a, b, c, c, d, a});
            }
        }

        data.vertex_count = static_cast<uint32_t>(data.vertices.size());
        data.indice_count = static_cast<uint32_t>(data.indices.size());
        return data;
    }

    MeshData CreateCapsule(float radius, float height, int segments) {
        return CreateCapsule(radius, height, segments, kWhite);
    }

    MeshData CreateCapsule(float radius, float height, int segments, const Colour& colour) {
        segments = std::max(6, segments);
        const int rings = std::max(4, segments / 2);
        const float cylinderHeight = std::max(0.0f, height - radius * 2.0f);

        MeshData data;
        MeshData top = CreateSphere(radius, segments, rings, colour);
        MeshData bottom = CreateSphere(radius, segments, rings, colour);
        MeshData body = CreateCylinder(radius, cylinderHeight, segments, colour);

        for (auto& vertex : top.vertices) {
            vertex.position.y += cylinderHeight * 0.5f;
        }
        for (auto& vertex : bottom.vertices) {
            vertex.position.y = -vertex.position.y - cylinderHeight * 0.5f;
            vertex.normal.y = -vertex.normal.y;
        }

        data.vertices.reserve(top.vertices.size() + bottom.vertices.size() + body.vertices.size());
        data.indices.reserve(top.indices.size() + bottom.indices.size() + body.indices.size());

        auto appendMesh = [&data](const MeshData& source) {
            const uint32_t base = static_cast<uint32_t>(data.vertices.size());
            data.vertices.insert(data.vertices.end(), source.vertices.begin(), source.vertices.end());
            for (uint32_t index : source.indices) {
                data.indices.push_back(base + index);
            }
        };

        appendMesh(top);
        appendMesh(body);
        appendMesh(bottom);

        data.vertex_count = static_cast<uint32_t>(data.vertices.size());
        data.indice_count = static_cast<uint32_t>(data.indices.size());
        return data;
    }
}
