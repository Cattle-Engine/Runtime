
#version 450

layout(location = 0) in vec2 fragUV;
layout(set = 2, binding = 0) uniform sampler2D renderTexture;
layout(location = 0) out vec4 outColor;

const float exposure = 0.5;

vec3 ACESFilm(vec3 x) {
    return clamp(
        (x * (2.51 * x + 0.03)) /
        (x * (2.43 * x + 0.59) + 0.14),
        0.0,
        1.0
    );
}

vec3 LinearToSRGB(vec3 x) {
    vec3 low = x * 12.92;
    vec3 high = 1.055 * pow(x, vec3(1.0 / 2.4)) - 0.055;

    return mix(low, high, step(vec3(0.0031308), x));
}

void main() {
    vec4 hdr = texture(renderTexture, fragUV);

    vec3 exposed = hdr.rgb * exposure;
    vec3 mapped = ACESFilm(exposed);
    vec3 srgb = LinearToSRGB(mapped);

    outColor = vec4(srgb, hdr.a);
}
