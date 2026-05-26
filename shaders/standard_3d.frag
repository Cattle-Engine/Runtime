#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec2 fragUV;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

layout(set = 3, binding = 0) uniform LightingUBO {
    vec4 sunDirectionEnabled;
    vec4 sunColourIntensity;
    vec4 ambientColourIntensity;
    vec4 materialTint;
    vec4 materialProps;
    vec4 cameraPositionShininess;
};

layout(location = 0) out vec4 outColor;

void main() {
    vec4 albedoSample = texture(texSampler, fragUV) * fragColor * materialTint;
    vec3 albedo = albedoSample.rgb;
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(cameraPositionShininess.xyz - fragWorldPos);

    vec3 ambient = ambientColourIntensity.rgb * ambientColourIntensity.a * albedo;
    vec3 lighting = ambient;

    if (sunDirectionEnabled.w > 0.5) {
        vec3 lightDir = normalize(-sunDirectionEnabled.xyz);
        float diffuseStrength = max(dot(normal, lightDir), 0.0);

        vec3 halfwayDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(normal, halfwayDir), 0.0);
        float shininess = max(materialProps.z, 1.0);
        float specularStrength = pow(specAngle, shininess);

        vec3 lightColour = sunColourIntensity.rgb * sunColourIntensity.a;
        vec3 diffuse = diffuseStrength * lightColour * albedo;

        float metallic = clamp(materialProps.y, 0.0, 1.0);
        vec3 baseSpecular = mix(vec3(0.04), albedo, metallic);
        vec3 specular = specularStrength * lightColour * baseSpecular;

        lighting += diffuse + specular;
    }

    outColor = vec4(lighting, albedoSample.a);
}
