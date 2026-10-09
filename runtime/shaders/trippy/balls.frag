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

void main() {
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    float time = misc.x;

    uv -= 0.5;

    // Make everything breathe and wobble.
    uv.x += sin(uv.y * 12.0 + time * 1.7) * 0.08;
    uv.y += cos(uv.x * 10.0 + time * 1.3) * 0.08;

    float r = length(uv);

    // Swirly psychedelic distortion.
    float angle = atan(uv.y, uv.x);
    angle += sin(r * 18.0 - time * 2.0) * 0.8;
    angle += sin(r * 7.0 + time) * 0.4;

    vec2 warped;
    warped.x = cos(angle) * r;
    warped.y = sin(angle) * r;

    float wave1 = sin(warped.x * 14.0 + time * 2.0);
    float wave2 = sin(warped.y * 17.0 - time * 1.5);
    float wave3 = sin((warped.x + warped.y) * 20.0 + time);

    float pattern = wave1 + wave2 + wave3;
    pattern = pattern / 3.0;

    // Psychedelic colour cycling.
    vec3 color;
    color.r = 0.5 + 0.5 * sin(pattern * 5.0 + time * 1.1);
    color.g = 0.5 + 0.5 * sin(pattern * 5.0 + time * 1.1 + 2.094);
    color.b = 0.5 + 0.5 * sin(pattern * 5.0 + time * 1.1 + 4.188);

    // Glowy centre.
    float glow = 1.0 - smoothstep(0.0, 0.7, r);
    color *= 0.7 + glow * 0.8;

    // Slowly breathe the whole thing.
    color *= 0.85 + 0.15 * sin(time * 0.8);

    outColor = vec4(color, 1.0);
}