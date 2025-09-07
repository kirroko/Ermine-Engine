#version 410

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture0;

// Shading mode toggle
uniform bool isBlinnPhong;

// Material properties for Blinn-Phong
uniform vec3 materialKa;
uniform vec3 materialKd;
uniform vec3 materialKs;
uniform float materialShininess;

// PBR Material properties
uniform vec3 pbrAlbedo;
uniform float pbrMetallic;
uniform float pbrRoughness;
uniform float pbrAO;

// Basic lighting (simplified - no UBO for compatibility)
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

const float PI = 3.14159265359;

// PBR Functions
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    // Basic texture sampling
    vec4 texColor = texture(texture0, TexCoord);
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    vec3 result;
    
    if (isBlinnPhong) {
        // Blinn-Phong Lighting
        // Ambient
        vec3 ambient = materialKa * lightColor * 0.1;

        // Diffuse
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor * materialKd;

        // Specular (Blinn-Phong)
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), materialShininess);
        vec3 specular = spec * lightColor * materialKs;

        result = (ambient + diffuse + specular) * texColor.rgb;
    } else {
        // PBR Lighting
        vec3 albedo = texColor.rgb * pbrAlbedo;
        vec3 F0 = vec3(0.04);
        F0 = mix(F0, albedo, pbrMetallic);
        
        vec3 H = normalize(viewDir + lightDir);
        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColor * attenuation;
        
        float NDF = DistributionGGX(norm, H, pbrRoughness);
        float G = GeometrySmith(norm, viewDir, lightDir, pbrRoughness);
        vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - pbrMetallic;
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(norm, viewDir), 0.0) * max(dot(norm, lightDir), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        float NdotL = max(dot(norm, lightDir), 0.0);
        vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
        
        vec3 ambient = vec3(0.03) * albedo * pbrAO;
        result = ambient + Lo;
    }
    
    // Gamma correction
    result = pow(result, vec3(1.0/2.2));
    
    FragColor = vec4(result, texColor.a);
}