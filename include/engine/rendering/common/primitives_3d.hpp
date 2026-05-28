#pragma once

#include <glm/glm.hpp>
#include "engine/rendering/renderer.hpp"

namespace CE::Renderer::Primitives3D {
    MeshData CreateCube(glm::vec3 size = {1.0f, 1.0f, 1.0f});
    MeshData CreateCube(glm::vec3 size, const Colour& colour);

    MeshData CreatePlane(glm::vec2 size = {1.0f, 1.0f});
    MeshData CreatePlane(glm::vec2 size, const Colour& colour);

    MeshData CreateSphere(float radius = 0.5f, int segments = 16, int rings = 16);
    MeshData CreateSphere(float radius, int segments, int rings, const Colour& colour);

    MeshData CreateCapsule(float radius = 0.5f, float height = 1.0f, int segments = 16);
    MeshData CreateCapsule(float radius, float height, int segments, const Colour& colour);

    MeshData CreateCylinder(float radius = 0.5f, float height = 1.0f, int segments = 16);
    MeshData CreateCylinder(float radius, float height, int segments, const Colour& colour);

    MeshData CreateTorus(float radius = 1.0f, float tubeRadius = 0.25f, int segments = 24, int tubeSegments = 12);
    MeshData CreateTorus(float radius, float tubeRadius, int segments, int tubeSegments, const Colour& colour);

    MeshData CreateCone(float radius = 0.5f, float height = 1.0f, int segments = 16);
    MeshData CreateCone(float radius, float height, int segments, const Colour& colour);

    void SetMeshColour(MeshData& mesh, const Colour& colour);
}