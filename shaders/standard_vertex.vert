#version 450

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vOvalPos;

layout(set = 0, binding = 0) uniform UBO
{
    vec2 Position;
} ubo;

const vec2 cVertexPositions[3] = vec2[](
    vec2(-1.0,  3.0),
    vec2( 3.0, -1.0),
    vec2(-1.0, -1.0)
);

const vec2 cTextureCoordinates[3] = vec2[](
    vec2(0.0, 2.0),
    vec2(2.0, 0.0),
    vec2(0.0, 0.0)
);

void main()
{
    int vertexIndex = gl_VertexID % 3;

    vTexCoord = cTextureCoordinates[vertexIndex];
    vOvalPos  = ubo.Position;

    gl_Position = vec4(cVertexPositions[vertexIndex], 0.0, 1.0);
}