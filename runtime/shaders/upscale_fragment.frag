
#version 450

layout(location = 0) in vec2 fragUV;
layout(set = 2, binding = 0) uniform sampler2D renderTexture;
layout(location = 0) out vec4 outColor;

const float exposure = 0.4555;

vec3 ACESFilm(vec3 x) {
    return clamp(
        (x * (2.51 * x + 0.03)) /
        (x * (2.43 * x + 0.59) + 0.14),
        0.0,
        1.0
    );
}

void main() {
    vec4 hdr = texture(renderTexture, fragUV);

    vec3 exposed = hdr.rgb * exposure;
    vec3 mapped = ACESFilm(exposed);

    // Convert linear RGB to sRGB for display.
    vec3 srgb = pow(mapped, vec3(1.0 / 2.2));

    outColor = vec4(srgb, hdr.a);
}
