#version 450

layout(location = 0) in vec2 vUV;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform Params {
    float uTime;
};

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(
        mix(a, b, f.x),
        mix(c, d, f.x),
        f.y
    );
}

void main() {
    vec2 uv = vUV * 4.0;

    // Move the noise over time.
    uv += vec2(uTime * 0.2, uTime * 0.1);

    float n = noise(uv);

    vec3 colour = vec3(n);

    outColor = vec4(colour, 1.0);
}