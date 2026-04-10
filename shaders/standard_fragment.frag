#version 450

layout(set = 0, binding = 0) uniform ColorBlock {
    vec4 color;
} uColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = uColor.color;
}