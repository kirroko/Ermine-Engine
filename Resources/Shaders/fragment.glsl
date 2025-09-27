#version 460

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 ViewPos; // Position in view space
in vec3 Tangent;   // Add for normal mapping
in vec3 Bitangent; // Add for normal mapping

out vec4 FragColor;

// Material uniform block
layout (std140) uniform MaterialBlock {
    vec4 albedo; // Changed from vec3 to vec4 to include alpha channel
    float metallic;
    float roughness;
    float ao;
    vec3 emissive;
    float emissiveIntensity;
    float normalStrength;
    int shadingModel; // 0 = PBR, 1 = Blinn-Phong
    
    // Flags for texture presence
    bool hasAlbedoMap;
    bool hasNormalMap;
    bool hasRoughnessMap;
    bool hasMetallicMap;
    bool hasAoMap;
    bool hasEmissiveMap;
    bool hasEnvironmentMap;
    bool hasIrradianceMap;
    
    // Environment mapping parameters
    float reflectance;
    float environmentIntensity;
} material;

// Separate texture samplers (cannot be in uniform blocks)
uniform sampler2D materialAlbedoMap;
uniform sampler2D materialNormalMap;
uniform sampler2D materialRoughnessMap;
uniform sampler2D materialMetallicMap;
uniform sampler2D materialAoMap;
uniform sampler2D materialEmissiveMap;

// Environment mapping samplers
uniform samplerCube materialEnvironmentMap;  // Main environment/reflection map
uniform samplerCube materialIrradianceMap;   // Irradiance map for diffuse IBL

// Shading mode toggle
uniform bool isBlinnPhong;

// View matrix for world space calculations
uniform mat4 view;

// Material properties for Blinn-Phong
uniform vec3 materialKa = vec3(0.2, 0.2, 0.2);
uniform vec3 materialKd = vec3(0.8, 0.8, 0.8);
uniform vec3 materialKs = vec3(1.0, 1.0, 1.0);
uniform vec3 materialKe = vec3(0.0, 0.0, 0.0);
uniform float materialShininess = 64.0;

// Legacy uniforms for backwards compatibility
uniform vec3 pbrAlbedo = vec3(0.8, 0.8, 0.8);
uniform float pbrMetallic = 0.0;
uniform float pbrRoughness = 0.5;
uniform float pbrAO = 1.0;
uniform vec3 pbrEmissive = vec3(0.0);
uniform float pbrEmissiveIntensity = 0.0;

struct Light {
    vec4 position_type;    // xyz = position (view space), w = light type
    vec4 color_intensity;  // xyz = color, w = intensity
    vec4 direction_range;  // xyz = direction (view space), w = range
    vec4 spot_angles;      // x = inner cos, y = outer cos
};

// SSBO for multiple lights
layout (std430, binding = 1) restrict readonly buffer LightsSSBO {
    vec4 lightCount;      // x = number of lights
    Light lights[];       // array of Light structs
};

const float PI = 3.14159265359;
const int POINT_LIGHT = 0;
const int DIRECTIONAL_LIGHT = 1;
const int SPOT_LIGHT = 2;

// Normal mapping function
vec3 calculateNormal()
{
    vec3 normal = normalize(Normal);
    
    if (material.hasNormalMap) {
        // Sample normal map
        vec3 normalMap = texture(materialNormalMap, TexCoord).rgb * 2.0 - 1.0;
        normalMap.xy *= material.normalStrength;
        
        // Create TBN matrix
        vec3 T = normalize(Tangent);
        vec3 B = normalize(Bitangent);
        vec3 N = normal;
        mat3 TBN = mat3(T, B, N);
        
        normal = normalize(TBN * normalMap);
    }
    
    return normal;
}

// Sample material properties with texture support
vec3 getAlbedo()
{
    vec3 albedo = material.albedo.rgb; // Use only RGB components
    
    if (material.hasAlbedoMap) {
        vec4 texColor = texture(materialAlbedoMap, TexCoord);
        albedo *= texColor.rgb;
    }
    
    return albedo;
}

float getRoughness()
{
    float roughness = material.roughness;
    if (material.hasRoughnessMap) {
        roughness *= texture(materialRoughnessMap, TexCoord).r;
    }
    return clamp(roughness, 0.05, 1.0);
}

float getMetallic()
{
    float metallic = material.metallic;
    if (material.hasMetallicMap) {
        metallic *= texture(materialMetallicMap, TexCoord).r;
    }
    return clamp(metallic, 0.0, 1.0);
}

float getAO()
{
    float ao = material.ao;
    if (material.hasAoMap) {
        ao *= texture(materialAoMap, TexCoord).r;
    }
    return ao;
}

vec3 getEmissive()
{
    vec3 emissive = material.emissive * material.emissiveIntensity;
    if (material.hasEmissiveMap) {
        vec4 emissiveTexel = texture(materialEmissiveMap, TexCoord);
        emissive *= emissiveTexel.rgb;
    }
    return emissive;
}

// PBR Functions (same as before)
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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

// Calculate light attenuation and spot effect
float calculateAttenuation(int lightIndex, vec3 fragPosView, out vec3 lightDir)
{
    int lightType = int(lights[lightIndex].position_type.w);
    vec3 lightPosView = lights[lightIndex].position_type.xyz;
    float range = lights[lightIndex].direction_range.w;
    
    float attenuation = 1.0;
    
    if (lightType == DIRECTIONAL_LIGHT) {
        lightDir = normalize(-lights[lightIndex].direction_range.xyz);
        attenuation = 1.0;
    } else {
        lightDir = normalize(lightPosView - fragPosView);
        float distance = length(lightPosView - fragPosView);
        
        float linearTerm = 0.045;
        float quadraticTerm = 0.0075;
        attenuation = 1.0 / (1.0 + linearTerm * distance + quadraticTerm * distance * distance);
        
        if (distance > range) {
            float fadeDistance = range * 0.1;
            float fadeStart = range - fadeDistance;
            if (distance > fadeStart) {
                float fadeFactor = 1.0 - (distance - fadeStart) / fadeDistance;
                attenuation *= max(fadeFactor, 0.0);
            } else {
                attenuation = 0.0;
            }
        }
        
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

// Environment mapping functions
vec3 calculateEnvironmentReflection(vec3 normal, vec3 viewDir, float roughness, vec3 F0)
{
    if (!material.hasEnvironmentMap) return vec3(0.0);
    
    // Convert from view space to world space for environment mapping
    mat3 viewToWorld = transpose(mat3(view));
    vec3 worldNormal = viewToWorld * normal;
    vec3 worldViewDir = viewToWorld * viewDir;
    
    // Calculate reflection vector in world space
    vec3 R = reflect(-worldViewDir, worldNormal);
    
    // Sample environment map with mipmap level based on roughness
    float mipLevel = roughness * 8.0; // Assuming 8 mip levels
    vec3 envReflection = textureLod(materialEnvironmentMap, R, mipLevel).rgb;
    
    return envReflection * material.environmentIntensity;
}

vec3 calculateEnvironmentIrradiance(vec3 normal)
{
    if (!material.hasIrradianceMap) return vec3(0.0);
    
    // Convert normal from view space to world space
    mat3 viewToWorld = transpose(mat3(view));
    vec3 worldNormal = viewToWorld * normal;
    
    // Sample irradiance map for diffuse environment lighting
    vec3 irradiance = texture(materialIrradianceMap, worldNormal).rgb;
    
    return irradiance * material.environmentIntensity;
}

// Blinn-Phong lighting calculation for one light
vec3 calculateBlinnPhong(int lightIndex, vec3 normal, vec3 viewDir, vec3 fragPosView, vec3 albedo)
{
    vec3 lightDir;
    float attenuation = calculateAttenuation(lightIndex, fragPosView, lightDir);
    
    if (attenuation <= 0.0) return vec3(0.0);
    
    vec3 lightColor = lights[lightIndex].color_intensity.xyz * lights[lightIndex].color_intensity.w;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * materialKd * albedo;
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec;
    if (materialShininess <= 0.0) {
        spec = 0.0;
    } else {
        spec = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);
    }
    vec3 specular = spec * lightColor * materialKs;
    
    return (diffuse + specular) * attenuation;
}

// PBR lighting calculation for one light
vec3 calculatePBR(int lightIndex, vec3 normal, vec3 viewDir, vec3 fragPosView, vec3 albedo, vec3 F0, float roughness, float metallic)
{
    vec3 lightDir;
    float attenuation = calculateAttenuation(lightIndex, fragPosView, lightDir);
    
    if (attenuation <= 0.0) return vec3(0.0);
    
    vec3 lightColor = lights[lightIndex].color_intensity.xyz * lights[lightIndex].color_intensity.w;
    vec3 radiance = lightColor * attenuation;
    
    vec3 H = normalize(viewDir + lightDir);
    
    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main()
{
    // Calculate normal (with potential normal mapping)
    vec3 norm = calculateNormal();
    vec3 viewDir = normalize(-ViewPos);
    
    // Sample material properties
    vec3 albedo = getAlbedo();
    float roughness = getRoughness();
    float metallic = getMetallic();
    float ao = getAO();
    vec3 emissive = getEmissive();
    
    vec3 result = vec3(0.0);
    int numLights = int(lightCount.x);
    
    // Choose shading model based on material settings or global toggle
    bool useBlinnPhong = isBlinnPhong || (material.shadingModel == 1);
    
    if (useBlinnPhong) {
        // Ambient component
        vec3 ambient = materialKa * 0.1 * albedo * ao;
        
        // Add environment irradiance as ambient if available
        if (material.hasIrradianceMap) {
            ambient += calculateEnvironmentIrradiance(norm) * albedo * 0.3;
        }
        
        result += ambient;

        // Add contribution from each light
        for (int i = 0; i < numLights && i < 16; ++i) {
            result += calculateBlinnPhong(i, norm, viewDir, ViewPos, albedo);
        }
        
        // Add simple environment reflection for Blinn-Phong
        if (material.hasEnvironmentMap) {
            vec3 envReflection = calculateEnvironmentReflection(norm, viewDir, 0.2, vec3(0.04));
            result += envReflection * materialKs * 0.5; // Simple reflection contribution
        }
        
        // Add emissive (material system handles this now)
        result += emissive;
        // Legacy support
        result += materialKe;
    } else {
        // PBR Lighting with Image-Based Lighting (IBL)
        vec3 F0 = vec3(material.reflectance);
        F0 = mix(F0, albedo, metallic);
        
        // Ambient lighting from environment or fallback
        vec3 ambient = vec3(0.08) * albedo * ao;
        if (material.hasIrradianceMap) {
            vec3 kS = fresnelSchlickRoughness(max(dot(norm, viewDir), 0.0), F0, roughness);
            vec3 kD = 1.0 - kS;
            kD *= 1.0 - metallic;
            
            vec3 irradiance = calculateEnvironmentIrradiance(norm);
            vec3 diffuse = irradiance * albedo;
            ambient = (kD * diffuse) * ao;
        }
        
        result += ambient;

        // Add contribution from each light
        for (int i = 0; i < numLights && i < 16; ++i) {
            result += calculatePBR(i, norm, viewDir, ViewPos, albedo, F0, roughness, metallic);
        }
        
        // Environment reflections for PBR
        if (material.hasEnvironmentMap) {
            vec3 kS = fresnelSchlickRoughness(max(dot(norm, viewDir), 0.0), F0, roughness);
            vec3 envReflection = calculateEnvironmentReflection(norm, viewDir, roughness, F0);
            vec3 specular = envReflection * kS;
            result += specular * ao;
        }
        
        // Energy compensation for very rough surfaces
        if (roughness > 0.7) {
            result *= mix(1.0, 1.4, (roughness - 0.7) / 0.3);
        }
        
        // Add emissive
        result += emissive;
        // Legacy support
        result += pbrEmissive * pbrEmissiveIntensity;
    }
    
    // Tone mapping (ACES approximation)
    vec3 a = 2.51 * result;
    vec3 b = 0.03 + result;
    vec3 c = 2.43 * result + 0.59;
    vec3 d = 0.14 + result;
    result = clamp((a * b) / (c * d), 0.0, 1.0);
    
    // Gamma correction
    result = pow(result, vec3(1.0/2.2));
    
    // Use alpha from albedo for transparency
    float alpha = material.albedo.a;
    FragColor = vec4(result, alpha);
}