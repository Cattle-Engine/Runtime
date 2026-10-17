#version 450

layout(location = 0) in vec2 fragUV;
layout(set = 2, binding = 0) uniform sampler2D renderTexture;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 hdr = texture(renderTexture, fragUV);

    vec3 mapped = hdr.rgb / (hdr.rgb + vec3(1.0));

    outColor = vec4(mapped, hdr.a);
}