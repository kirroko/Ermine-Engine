#version 460

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 ViewPos; // Position in view space

out vec4 FragColor;

uniform sampler2D texture0;

// Shading mode toggle
uniform bool isBlinnPhong;

// Material properties for Blinn-Phong
uniform vec3 materialKa;
uniform vec3 materialKd;
uniform vec3 materialKs;
uniform vec3 materialKe;
uniform float materialShininess;

// PBR Material properties
uniform vec3 pbrAlbedo;
uniform float pbrMetallic;
uniform float pbrRoughness;
uniform float pbrAO;
uniform vec3 pbrEmissive;
uniform float pbrEmissiveIntensity;

uniform vec3 viewPos; // World space view position for PBR

struct Light {
    vec4 position_type;    // xyz = position (view space), w = light type
    vec4 color_intensity;  // xyz = color, w = intensity
    vec4 direction_range;  // xyz = direction (view space), w = range
    vec4 spot_angles;      // x = inner cos, y = outer cos
};

// UBO for multiple lights
layout (std140) uniform Lights {
    vec4 lightCount;      // x = number of lights
    Light lights[16];     // array of Light structs
};


const float PI = 3.14159265359;
const int POINT_LIGHT = 0;
const int DIRECTIONAL_LIGHT = 1;
const int SPOT_LIGHT = 2;

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

// Calculate light attenuation and spot effect
float calculateAttenuation(int lightIndex, vec3 fragPosView, out vec3 lightDir)
{
    int lightType = int(lights[lightIndex].position_type.w);
    vec3 lightPosView = lights[lightIndex].position_type.xyz;
    float range = lights[lightIndex].direction_range.w;
    
    float attenuation = 1.0;
    
    if (lightType == DIRECTIONAL_LIGHT) {
        // Directional light - direction is stored directly
        lightDir = normalize(-lights[lightIndex].direction_range.xyz);
        attenuation = 1.0; // No attenuation for directional lights
    } else {
        // Point or spot light - calculate direction from position
        lightDir = normalize(lightPosView - fragPosView);
        float distance = length(lightPosView - fragPosView);
        
        // Improved distance attenuation with configurable falloff
        float linearTerm = 0.045; // Reduced for less aggressive falloff
        float quadraticTerm = 0.0075; // Reduced for less aggressive falloff
        attenuation = 1.0 / (1.0 + linearTerm * distance + quadraticTerm * distance * distance);
        
        // Range cutoff with smooth transition
        if (distance > range) {
            float fadeDistance = range * 0.1; // 10% of range for smooth fade
            float fadeStart = range - fadeDistance;
            if (distance > fadeStart) {
                float fadeFactor = 1.0 - (distance - fadeStart) / fadeDistance;
                attenuation *= max(fadeFactor, 0.0);
            } else {
                attenuation = 0.0;
            }
        }
        
        // Spot light cone attenuation
        if (lightType == SPOT_LIGHT) {
            vec3 spotDir = normalize(lights[lightIndex].direction_range.xyz);
            float cosAngle = dot(-lightDir, spotDir);
            float innerCos = lights[lightIndex].spot_angles.x;
            float outerCos = lights[lightIndex].spot_angles.y;
            
            float spotFactor = clamp((cosAngle - outerCos) / (innerCos - outerCos), 0.0, 1.0);
            attenuation *= spotFactor;
        }
    }
    
    return attenuation;
}

// Blinn-Phong lighting calculation for one light
vec3 calculateBlinnPhong(int lightIndex, vec3 normal, vec3 viewDir, vec3 fragPosView)
{
    vec3 lightDir;
    float attenuation = calculateAttenuation(lightIndex, fragPosView, lightDir);
    
    if (attenuation <= 0.0) return vec3(0.0);
    
    vec3 lightColor = lights[lightIndex].color_intensity.xyz * lights[lightIndex].color_intensity.w;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * materialKd;
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);
    vec3 specular = spec * lightColor * materialKs;
    
    return (diffuse + specular) * attenuation;
}

// PBR lighting calculation for one light
vec3 calculatePBR(int lightIndex, vec3 normal, vec3 viewDir, vec3 fragPosView, vec3 albedo, vec3 F0)
{
    vec3 lightDir;
    float attenuation = calculateAttenuation(lightIndex, fragPosView, lightDir);
    
    if (attenuation <= 0.0) return vec3(0.0);
    
    vec3 lightColor = lights[lightIndex].color_intensity.xyz * lights[lightIndex].color_intensity.w;
    vec3 radiance = lightColor * attenuation;
    
    vec3 H = normalize(viewDir + lightDir);
    
    // Clamp roughness to prevent division by zero and artifacts
    float roughness = clamp(pbrRoughness, 0.05, 1.0);
    
    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - pbrMetallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main()
{
    vec4 texColor = texture(texture0, TexCoord);
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(-ViewPos); // ViewPos is already in view space
    
    vec3 result = vec3(0.0);
    int numLights = int(lightCount.x);
    
    if (isBlinnPhong) {
        // Ambient component (applied once)
        vec3 ambient = materialKa * 0.1;
        result += ambient * texColor.rgb;

        // Add contribution from each light
        for (int i = 0; i < numLights && i < 16; ++i) {
            result += calculateBlinnPhong(i, norm, viewDir, ViewPos) * texColor.rgb;
        }
        
        // Add Flat Emission
        result += materialKe; 
    } else {
        // PBR Lighting
        vec3 albedo = texColor.rgb * pbrAlbedo;
        
        // Improved F0 calculation
        vec3 F0 = vec3(0.04);
        F0 = mix(F0, albedo, pbrMetallic);
        
        // More balanced ambient lighting
        vec3 ambient = vec3(0.08) * albedo * pbrAO; // Reduced from 0.15 to 0.08
        result += ambient;

        // Add contribution from each light
        for (int i = 0; i < numLights && i < 16; ++i) {
            result += calculatePBR(i, norm, viewDir, ViewPos, albedo, F0);
        }
        
        // Additional energy compensation for very rough surfaces
        if (pbrRoughness > 0.7) {
            result *= mix(1.0, 1.4, (pbrRoughness - 0.7) / 0.3);
        }
        
        // Add Flat Emission
        result += pbrEmissive * pbrEmissiveIntensity;
    }
    
    // Improved tone mapping (ACES approximation)
    vec3 a = 2.51 * result;
    vec3 b = 0.03 + result;
    vec3 c = 2.43 * result + 0.59;
    vec3 d = 0.14 + result;
    result = clamp((a * b) / (c * d), 0.0, 1.0);
    
    // Gamma correction
    result = pow(result, vec3(1.0/2.2));
    
    FragColor = vec4(result, texColor.a);
}