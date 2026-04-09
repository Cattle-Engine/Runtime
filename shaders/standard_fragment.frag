#version 450

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vOvalPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UBO
{
    vec4 color;
} ubo;

void main()
{
    vec3 colors[2] = vec3[](
        ubo.color.xyz,
        vec3(0.0, 0.0, 0.0)
    );

    float distanceFromFragment = length(vTexCoord - vOvalPos);

    vec3 finalColor = (distanceFromFragment < 0.1)
        ? colors[0]
        : colors[1];

    outColor = vec4(finalColor, 1.0);
}