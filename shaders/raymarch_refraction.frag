#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;

layout(set = 2, binding = 0) uniform sampler2D texSampler0;
layout(set = 2, binding = 1) uniform sampler2D texSampler1;
layout(set = 2, binding = 2) uniform sampler2D texSampler2;
layout(set = 2, binding = 3) uniform sampler2D texSampler3;

layout(set = 3, binding = 0) uniform FragmentUserData {
    vec4 tint;
    vec4 resolution;
    vec4 misc;
    vec4 customVec4[8];
    ivec4 customInt4[4];
} ubo;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358979323846;

vec3 n4 = vec3(0.577, 0.577, 0.577);
vec3 n5 = vec3(-0.577, 0.577, 0.577);
vec3 n6 = vec3(0.577, -0.577, 0.577);
vec3 n7 = vec3(0.577, 0.577, -0.577);
vec3 n8 = vec3(0.000, 0.357, 0.934);
vec3 n9 = vec3(0.000, -0.357, 0.934);
vec3 n10 = vec3(0.934, 0.000, 0.357);
vec3 n11 = vec3(-0.934, 0.000, 0.357);
vec3 n12 = vec3(0.357, 0.934, 0.000);
vec3 n13 = vec3(-0.357, 0.934, 0.000);

float iTime() {
    return ubo.misc.x;
}

vec2 iResolution() {
    vec2 resolution = ubo.resolution.xy;
    if (resolution.x <= 0.0 || resolution.y <= 0.0) {
        return vec2(1280.0, 720.0);
    }
    return resolution;
}

vec2 rotate2D(vec2 p, float a) {
    return p * mat2(cos(a), -sin(a), sin(a), cos(a));
}

float sdBox(vec3 position, vec3 dimensions) {
    vec3 d = abs(position) - dimensions;
    return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float randomValue(vec2 co) {
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(co.xy, vec2(a, b));
    float sn = mod(dt, 3.14);
    return fract(sin(sn) * c);
}

float fogFactorExp2(const float dist, const float density) {
    const float LOG2 = -1.442695;
    float d = density * dist;
    return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

float intersectPlane(vec3 ro, vec3 rd, vec3 nor, float dist) {
    float denom = dot(rd, nor);
    return -(dot(ro, nor) + dist) / denom;
}

float icosahedral(vec3 p, float r) {
    float s = abs(dot(p, n4));
    s = max(s, abs(dot(p, n5)));
    s = max(s, abs(dot(p, n6)));
    s = max(s, abs(dot(p, n7)));
    s = max(s, abs(dot(p, n8)));
    s = max(s, abs(dot(p, n9)));
    s = max(s, abs(dot(p, n10)));
    s = max(s, abs(dot(p, n11)));
    s = max(s, abs(dot(p, n12)));
    s = max(s, abs(dot(p, n13)));
    return s - r;
}

vec2 mapRefract(vec3 p) {
    float d = icosahedral(p, 1.0);
    return vec2(d, 0.0);
}

vec2 mapSolid(vec3 p) {
    p.xz = rotate2D(p.xz, iTime() * 1.25);
    p.yx = rotate2D(p.yx, iTime() * 1.85);
    p.y += sin(iTime()) * 0.25;
    p.x += cos(iTime()) * 0.25;

    float d = length(p) - 0.25;
    float pulse = pow(sin(iTime() * 2.0) * 0.5 + 0.5, 9.0) * 2.0;
    d = mix(d, sdBox(p, vec3(0.175)), pulse);

    return vec2(d, 1.0);
}

vec2 calcRayIntersectionRefract(vec3 rayOrigin, vec3 rayDir, float maxd, float precis) {
    float latest = precis * 2.0;
    float dist = 0.0;
    float type = -1.0;
    vec2 res = vec2(-1.0, -1.0);

    for (int i = 0; i < 50; i++) {
        if (latest < precis || dist > maxd) {
            break;
        }

        vec2 result = mapRefract(rayOrigin + rayDir * dist);
        latest = result.x;
        type = result.y;
        dist += latest;
    }

    if (dist < maxd) {
        res = vec2(dist, type);
    }

    return res;
}

vec2 calcRayIntersectionSolid(vec3 rayOrigin, vec3 rayDir, float maxd, float precis) {
    float latest = precis * 2.0;
    float dist = 0.0;
    float type = -1.0;
    vec2 res = vec2(-1.0, -1.0);

    for (int i = 0; i < 60; i++) {
        if (latest < precis || dist > maxd) {
            break;
        }

        vec2 result = mapSolid(rayOrigin + rayDir * dist);
        latest = result.x;
        type = result.y;
        dist += latest;
    }

    if (dist < maxd) {
        res = vec2(dist, type);
    }

    return res;
}

vec3 calcNormalRefract(vec3 pos, float eps) {
    const vec3 v1 = vec3(1.0, -1.0, -1.0);
    const vec3 v2 = vec3(-1.0, -1.0, 1.0);
    const vec3 v3 = vec3(-1.0, 1.0, -1.0);
    const vec3 v4 = vec3(1.0, 1.0, 1.0);

    return normalize(
        v1 * mapRefract(pos + v1 * eps).x +
        v2 * mapRefract(pos + v2 * eps).x +
        v3 * mapRefract(pos + v3 * eps).x +
        v4 * mapRefract(pos + v4 * eps).x
    );
}

vec3 calcNormalSolid(vec3 pos, float eps) {
    const vec3 v1 = vec3(1.0, -1.0, -1.0);
    const vec3 v2 = vec3(-1.0, -1.0, 1.0);
    const vec3 v3 = vec3(-1.0, 1.0, -1.0);
    const vec3 v4 = vec3(1.0, 1.0, 1.0);

    return normalize(
        v1 * mapSolid(pos + v1 * eps).x +
        v2 * mapSolid(pos + v2 * eps).x +
        v3 * mapSolid(pos + v3 * eps).x +
        v4 * mapSolid(pos + v4 * eps).x
    );
}

float beckmannDistribution(float x, float roughness) {
    float NdotH = max(x, 0.0001);
    float cos2Alpha = NdotH * NdotH;
    float tan2Alpha = (cos2Alpha - 1.0) / cos2Alpha;
    float roughness2 = roughness * roughness;
    float denom = PI * roughness2 * cos2Alpha * cos2Alpha;
    return exp(tan2Alpha / roughness2) / denom;
}

float cookTorranceSpecular(
    vec3 lightDirection,
    vec3 viewDirection,
    vec3 surfaceNormal,
    float roughness,
    float fresnel
) {
    float VdotN = max(dot(viewDirection, surfaceNormal), 0.0);
    float LdotN = max(dot(lightDirection, surfaceNormal), 0.0);

    vec3 H = normalize(lightDirection + viewDirection);
    float NdotH = max(dot(surfaceNormal, H), 0.0);
    float VdotH = max(dot(viewDirection, H), 0.000001);
    float LdotH = max(dot(lightDirection, H), 0.000001);
    float G1 = (2.0 * NdotH * VdotN) / VdotH;
    float G2 = (2.0 * NdotH * LdotN) / LdotH;
    float G = min(1.0, min(G1, G2));
    float D = beckmannDistribution(NdotH, roughness);
    float F = pow(1.0 - VdotN, fresnel);

    return G * F * D / max(PI * VdotN, 0.000001);
}

vec2 squareFrame(vec2 screenSize, vec2 coord) {
    vec2 position = 2.0 * (coord.xy / screenSize.xy) - 1.0;
    position.x *= screenSize.x / screenSize.y;
    return position;
}

mat3 calcLookAtMatrix(vec3 origin, vec3 target, float roll) {
    vec3 rr = vec3(sin(roll), cos(roll), 0.0);
    vec3 ww = normalize(target - origin);
    vec3 uu = normalize(cross(ww, rr));
    vec3 vv = normalize(cross(uu, ww));
    return mat3(uu, vv, ww);
}

vec3 getRay(vec3 origin, vec3 target, vec2 screenPos, float lensLength) {
    mat3 camMat = calcLookAtMatrix(origin, target, 0.0);
    return normalize(camMat * vec3(screenPos, lensLength));
}

void orbitCamera(
    float camAngle,
    float camHeight,
    float camDistance,
    vec2 screenResolution,
    out vec3 rayOrigin,
    out vec3 rayDirection,
    vec2 coord
) {
    vec2 screenPos = squareFrame(screenResolution, coord);
    vec3 rayTarget = vec3(0.0);

    rayOrigin = vec3(
        camDistance * sin(camAngle),
        camHeight,
        camDistance * cos(camAngle)
    );

    rayDirection = getRay(rayOrigin, rayTarget, screenPos, 2.0);
}

vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d) {
    return a + b * cos(6.28318 * (c * t + d));
}

vec3 bg(vec3 ro, vec3 rd) {
    vec3 col = 0.1 + palette(
        clamp((randomValue(rd.xz + sin(iTime() * 0.1)) * 0.5 + 0.5) * 0.035 - rd.y * 0.5 + 0.35, -1.0, 1.0),
        vec3(0.5, 0.45, 0.55),
        vec3(0.5, 0.5, 0.5),
        vec3(1.05, 1.0, 1.0),
        vec3(0.275, 0.2, 0.19)
    );

    float t = intersectPlane(ro, rd, vec3(0.0, 1.0, 0.0), 4.0);
    if (t > 0.0) {
        vec3 p = ro + rd * t;
        float g = 1.0 - pow(abs(sin(p.x) * cos(p.z)), 0.25);
        col += (1.0 - fogFactorExp2(t, 0.04)) * g * vec3(5.0, 4.0, 2.0) * 0.075;
    }

    return col;
}

void main() {
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 resolution = iResolution();
    vec2 uv = squareFrame(resolution, fragCoord);

    float dist = 4.5;
    float rotation = iTime() * 0.45;
    float height = -0.2 + sin(iTime() * 0.35) * 0.2;

    vec3 ro;
    vec3 rd;
    orbitCamera(rotation, height, dist, resolution, ro, rd, fragCoord);

    vec3 color = bg(ro, rd);
    vec2 t = calcRayIntersectionRefract(ro, rd, 20.0, 0.001);

    if (t.x > -0.5) {
        vec3 pos = ro + rd * t.x;
        vec3 nor = calcNormalRefract(pos, 0.002);
        vec3 ldir1 = normalize(vec3(0.8, 1.0, 0.0));
        vec3 ldir2 = normalize(vec3(-0.4, -1.3, 0.0));
        vec3 lcol1 = vec3(0.6, 0.5, 1.1);
        vec3 lcol2 = vec3(1.4, 0.9, 0.8) * 0.7;

        vec3 ref = refract(rd, nor, 0.97);
        vec2 u = calcRayIntersectionSolid(ro + ref * 0.1, ref, 20.0, 0.001);
        if (u.x > -0.5) {
            vec3 pos2 = ro + ref * u.x;
            vec3 nor2 = calcNormalSolid(pos2, 0.002);
            float spec = cookTorranceSpecular(ldir1, -ref, nor2, 0.6, 0.95) * 2.0;
            float diff1 = 0.05 + max(0.0, dot(ldir1, nor2));
            float diff2 = max(0.0, dot(ldir2, nor2));
            color = spec + (diff1 * lcol1 + diff2 * lcol2);
        } else {
            color = bg(ro + ref * 0.1, ref) * 1.1;
        }

        color += color * cookTorranceSpecular(ldir1, -rd, nor, 0.2, 0.9) * 2.0;
        color += 0.05;
    }

    float vignette = 1.0 - max(0.0, dot(uv * 0.155, uv));

    color.r = smoothstep(0.05, 0.995, color.r);
    color.g = smoothstep(-0.1, 0.95, color.g);
    color.b = smoothstep(-0.05, 0.95, color.b);
    color.b *= vignette;

    color *= ubo.tint.rgb;
    outColor = vec4(color, clamp(t.x, 0.5, 1.0) * ubo.tint.a);
}
