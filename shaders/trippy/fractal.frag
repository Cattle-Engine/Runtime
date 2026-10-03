#version 450

layout(location = 0) out vec4 outColor;

layout(set = 3, binding = 0, std140) uniform FragmentShaderUserData {
    vec4 tint;
    vec4 resolution;
    vec4 misc;
    vec4 customVec4[8];
    ivec4 customInt4[4];
    vec4 sunDirectionEnabled;
    vec4 sunColourIntensity;
    vec4 ambientColourIntensity;
    vec4 materialTint;
    vec4 materialProps;
    vec4 cameraPositionShininess;
    vec4 normalExists;
};

mat2 rotate(float a) {
    float c = cos(a);
    float s = sin(a);

    return mat2(
        c, -s,
        s,  c
    );
}

float sdCircle(vec2 p, float r) {
    return length(p) - r;
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    uv = uv * 2.0 - 1.0;

    float aspect = resolution.x / resolution.y;
    uv.x *= aspect;

    float time = misc.x;

    vec2 p = uv;

    // The whole image slowly folds in on itself.
    p *= 1.0 + sin(time * 0.7) * 0.08;

    float total = 0.0;
    float glow = 0.0;

    // Kaleidoscope.
    float a = atan(p.y, p.x);
    float r = length(p);

    const float PI = 3.14159265;

    float segments = 9.0;
    float sector = 2.0 * PI / segments;

    a = mod(a + sector * 0.5, sector) - sector * 0.5;

    p = vec2(cos(a), sin(a)) * r;

    // Iterated geometric distortion.
    for (int i = 0; i < 8; ++i) {
        float fi = float(i);

        p = abs(p);

        p -= vec2(
            0.32 + sin(time * 0.4 + fi) * 0.06,
            0.18 + cos(time * 0.3 + fi * 1.7) * 0.05
        );

        p *= rotate(
            0.35
            + sin(time * 0.25 + fi * 0.8) * 0.18
        );

        float scale = 1.35 + sin(time * 0.15 + fi) * 0.08;

        p *= scale;

        float d = length(p);

        total += sin(
            d * 12.0
            + fi * 1.8
            - time * (1.0 + fi * 0.08)
        ) / (1.0 + d * 8.0);

        glow += 0.015 / (0.02 + abs(d - 0.22));
    }

    // Wormhole rings.
    float wormhole =
        sin(
            r * 40.0
            - time * 4.0
            + sin(a * 12.0 + time) * 3.0
        );

    wormhole *= 0.5 + 0.5 * sin(
        r * 15.0 + time * 1.5
    );

    // Pulsating central void.
    float coreRadius =
        0.18
        + 0.035 * sin(time * 2.0);

    float core =
        smoothstep(
            coreRadius + 0.03,
            coreRadius - 0.03,
            r
        );

    // Build psychedelic colour waves.
    float v =
        total * 1.8
        + wormhole * 0.8
        + glow * 0.08;

    vec3 color;

    color.r =
        0.5 +
        0.5 * sin(v * 3.0 + time * 0.9);

    color.g =
        0.5 +
        0.5 * sin(v * 3.0 + time * 0.9 + 2.094);

    color.b =
        0.5 +
        0.5 * sin(v * 3.0 + time * 0.9 + 4.188);

    // Electric highlights.
    color += vec3(
        glow * 0.05,
        glow * 0.09,
        glow * 0.12
    );

    // A second moving colour field.
    vec3 secondary = vec3(
        sin(p.x * 9.0 + time),
        sin(p.y * 11.0 - time * 1.3),
        sin((p.x - p.y) * 13.0 + time * 0.7)
    );

    secondary = secondary * 0.5 + 0.5;

    color = mix(
        color,
        color * secondary * 1.8,
        0.35
    );

    // Make the centre feel like a portal.
    color += core * vec3(
        0.8 + 0.2 * sin(time),
        0.3 + 0.2 * sin(time + 2.0),
        1.0
    );

    // Pulsing exposure.
    float exposure =
        0.85
        + 0.25 * sin(time * 1.1)
        + 0.12 * sin(time * 3.7);

    color *= exposure;

    // Slight radial bloom.
    float bloom =
        exp(-r * 3.5)
        * (0.5 + 0.5 * sin(time * 2.0));

    color += bloom * vec3(
        0.25,
        0.05,
        0.35
    );

    // Contrast.
    color = color / (color + vec3(0.35));

    // Slowly rotate the final colour relationship.
    float channelShift =
        sin(time * 0.22 + r * 8.0);

    color.r += channelShift * 0.08;
    color.g += sin(channelShift * 2.0) * 0.08;
    color.b += cos(channelShift * 1.5) * 0.08;

    outColor = vec4(color, 1.0);
}