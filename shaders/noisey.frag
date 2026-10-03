#version 450

layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 3, binding = 0, std140) uniform FragmentShaderUserData {
    vec4 tint;
    vec4 resolution;
    vec4 misc;

    vec4 customVec4[8];
    ivec4 customInt4[4];
};

vec3 palette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.00, 0.33, 0.67);

    return a + b * cos(6.28318 * (c * t + d));
}

void main() {
    vec2 uv = fragUV;

    uv -= 0.5;
    uv.x *= 1.5;

    float time = misc.x;
    float t = time * 0.6;

    float wave1 = sin(uv.y * 8.0 + t * 2.0) * 0.08;
    float wave2 = sin(uv.x * 10.0 - t * 1.5) * 0.06;
    float wave3 = sin((uv.x + uv.y) * 14.0 + t) * 0.04;

    uv.x += wave1 + wave3;
    uv.y += wave2 + wave3;

    float flow = sin(
        uv.x * 6.0 +
        sin(uv.y * 5.0 + t) * 2.0 +
        t * 2.0
    );

    flow += sin(
        uv.y * 8.0 -
        uv.x * 4.0 -
        t * 1.5
    ) * 0.5;

    flow = flow * 0.5 + 0.5;

    vec3 color = palette(flow + t * 0.08);

    float brightness = 0.75 + 0.25 * sin(flow * 12.0 + t);

    color *= brightness;

    outColor = vec4(color, 1.0);
}