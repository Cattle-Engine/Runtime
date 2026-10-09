#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in float inTangentSign;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec2 fragUV;
layout(location = 4) out vec3 fragTangent;
layout(location = 5) out float fragTangentSign;

layout(set = 1, binding = 0) uniform CameraUBO {
    mat4 viewProjection;
    vec4 cameraPosition;
};

layout(set = 1, binding = 1) uniform ModelUBO {
    mat4 model;
    mat4 normalMatrix;
};

void main() {
    vec4 worldPos = model * vec4(inPos, 1.0);
    gl_Position = viewProjection * worldPos;

    fragWorldPos = worldPos.xyz;
    fragNormal = normalize((normalMatrix * vec4(inNormal, 0.0)).xyz);
    fragTangent = normalize((normalMatrix * vec4(inTangent, 0.0)).xyz);
    fragTangentSign = inTangentSign;
    fragColor = inColor;
    fragUV = inUV;
}