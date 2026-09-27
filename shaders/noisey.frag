#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec2 vUV;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in float fragTangentSign;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(vUV, 0.0, 1.0);
}