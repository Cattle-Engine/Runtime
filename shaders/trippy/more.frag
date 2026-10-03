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

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    uv = uv * 2.0 - 1.0;

    float aspect = resolution.x / resolution.y;
    uv.x *= aspect;

    float time = misc.x;

    // Slowly rotate the entire universe.
    float rotation = time * 0.12;

    float cs = cos(rotation);
    float sn = sin(rotation);

    uv = mat2(cs, -sn, sn, cs) * uv;

    vec2 originalUV = uv;

    float radius = length(uv);
    float angle = atan(uv.y, uv.x);

    // Recursive-feeling polar distortion.
    for (int i = 0; i < 5; ++i) {
        float fi = float(i);

        angle += sin(
            radius * (7.0 + fi * 3.0)
            - time * (0.7 + fi * 0.15)
        ) * (0.22 + fi * 0.035);

        radius += sin(
            angle * (5.0 + fi * 1.7)
            + time * (0.9 + fi * 0.2)
        ) * 0.008;
    }

    // Turn the distorted polar coordinates back into Cartesian space.
    vec2 p;
    p.x = cos(angle) * radius;
    p.y = sin(angle) * radius;

    // Breathing displacement.
    p.x += sin(p.y * 11.0 + time * 1.3) * 0.11;
    p.y += cos(p.x * 13.0 - time * 1.1) * 0.11;

    // Infinite-looking nested rings.
    float rings = sin(
        radius * 32.0
        - sin(angle * 6.0 + time) * 4.0
        - time * 2.5
    );

    // Interference pattern.
    float interference =
        sin(p.x * 17.0 + sin(p.y * 9.0 + time) * 4.0)
        +
        sin(p.y * 19.0 + sin(p.x * 11.0 - time) * 5.0);

    interference *= 0.5;

    // Strange central vortex.
    float vortex = sin(
        angle * 9.0
        + radius * 24.0
        - time * 3.0
    );

    // Combine the patterns.
    float pattern =
        rings * 0.45 +
        interference * 0.35 +
        vortex * 0.45;

    // Make the pattern fold back into itself.
    pattern = sin(pattern * 3.14159 + radius * 8.0);

    // Psychedelic colour space.
    float colourTime = time * 0.7;

    vec3 color;

    color.r = 0.5 + 0.5 * sin(pattern * 6.0 + colourTime);
    color.g = 0.5 + 0.5 * sin(pattern * 6.0 + colourTime + 2.094);
    color.b = 0.5 + 0.5 * sin(pattern * 6.0 + colourTime + 4.188);

    // Add another colour layer based on position.
    color += 0.25 * vec3(
        sin(p.x * 8.0 + time),
        sin(p.y * 9.0 - time * 1.2),
        sin((p.x + p.y) * 7.0 + time * 0.8)
    );

    // Chromatic vortex glow.
    float glow = 1.0 / (1.0 + radius * 5.0);

    color.r += glow * 0.45;
    color.g += glow * 0.20;
    color.b += glow * 0.55;

    // Pulsing hallucination.
    float pulse = 0.75 + 0.25 * sin(
        time * 1.7
        + radius * 15.0
        + pattern * 4.0
    );

    color *= pulse;

    // Bright psychedelic highlights.
    color = pow(max(color, vec3(0.0)), vec3(0.72));

    // Slowly rotate colour channels independently.
    float hueWave = sin(time * 0.35 + radius * 4.0);

    color.r += hueWave * 0.12;
    color.g += sin(hueWave * 3.0) * 0.12;
    color.b += cos(hueWave * 2.0) * 0.12;

    // Vignette, but make it pulse.
    float vignette = 1.0 - smoothstep(
        0.15,
        1.25,
        radius
    );

    vignette = mix(
        vignette,
        1.0,
        0.25 + 0.15 * sin(time)
    );

    color *= vignette;

    // Final colour feedback distortion.
    color += 0.12 * vec3(
        sin(color.g * 12.0 + time),
        sin(color.b * 13.0 - time),
        sin(color.r * 11.0 + time * 0.7)
    );

    outColor = vec4(color, 1.0);
}