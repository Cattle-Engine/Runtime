#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec2 fragUV;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in vec3 fragBitangent;

layout(set = 2, binding = 0) uniform sampler2D albedoSampler;
layout(set = 2, binding = 1) uniform sampler2D normalSampler;
layout(set = 2, binding = 2) uniform sampler2D metallicRoughnessSampler;

layout(set = 3, binding = 0) uniform LightingUBO {
    vec4 sunDirectionEnabled;
    vec4 sunColourIntensity;
    vec4 ambientColourIntensity;
    vec4 materialTint;
    vec4 materialProps;
    vec4 cameraPositionShininess;
};

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec4 albedoSample = texture(albedoSampler, fragUV) * fragColor * materialTint;
    vec3 albedo = albedoSample.rgb;
    
    vec3 tangentNormal = texture(normalSampler, fragUV).rgb * 2.0 - 1.0;
    vec3 N = normalize(fragNormal);
    vec3 T = normalize(fragTangent);
    vec3 B = normalize(fragBitangent);
    mat3 TBN = mat3(T, B, N);
    vec3 normal = normalize(TBN * tangentNormal);
    
    vec2 metallicRoughness = texture(metallicRoughnessSampler, fragUV).bg;
    float roughness = metallicRoughness.g * materialProps.x;
    float metallic = metallicRoughness.r * materialProps.y;
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);
    
    vec3 viewDir = normalize(cameraPositionShininess.xyz - fragWorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    vec3 ambient = ambientColourIntensity.rgb * ambientColourIntensity.a * albedo;
    vec3 lighting = ambient;
    
    if (sunDirectionEnabled.w > 0.5) {
        vec3 lightDir = normalize(-sunDirectionEnabled.xyz);
        vec3 halfwayDir = normalize(lightDir + viewDir);
        
        vec3 radiance = sunColourIntensity.rgb * sunColourIntensity.a;
        
        float NDF = DistributionGGX(normal, halfwayDir, roughness);
        float G = GeometrySmith(normal, viewDir, lightDir, roughness);
        vec3 F = FresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kD = (1.0 - F) * (1.0 - metallic);
        float NdotL = max(dot(normal, lightDir), 0.0);
        
        lighting += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    outColor = vec4(lighting, albedoSample.a);
}