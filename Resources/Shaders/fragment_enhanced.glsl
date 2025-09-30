#version 460

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 ViewPos; // Position in view space
in vec3 Tangent;   // For normal mapping
in vec3 Bitangent; // For normal mapping

out vec4 FragColor;

// Material uniform block - must match MaterialUBO structure exactly
layout (std140) uniform MaterialBlock {
    vec3 albedo;                  // 12 bytes + 4 padding = 16 bytes - changed back to vec3
    float metallic;               // 4 bytes (16-19)
    float roughness;              // 4 bytes (20-23)
    float ao;                     // 4 bytes (24-27)
    float normalStrength;         // 4 bytes (28-31)
    
    vec3 emissive;                // 16 bytes (32-47) - vec3 uses 16 bytes in std140
    float emissiveIntensity;      // 4 bytes (48-51)
    int shadingModel;             // 4 bytes (52-55) // 0 = PBR, 1 = Blinn-Phong
    float reflectance;            // 4 bytes (56-59)
    float environmentIntensity;   // 4 bytes (60-63)
    
    // Texture presence flags
    int hasAlbedoMap;             // 4 bytes (64-67)
    int hasNormalMap;             // 4 bytes (68-71)
    int hasRoughnessMap;          // 4 bytes (72-75)
    int hasMetallicMap;           // 4 bytes (76-79)
    
    int hasAoMap;                 // 4 bytes (80-83)
    int hasEmissiveMap;           // 4 bytes (84-87)
    int hasEnvironmentMap;        // 4 bytes (88-91)
    int hasIrradianceMap;         // 4 bytes (92-95)
    
    // Transparency parameters (moved from albedo.alpha to dedicated fields)
    float transparency;           // 4 bytes (96-99) - 0.0 = opaque, 1.0 = fully transparent
    float indexOfRefraction;      // 4 bytes (100-103)
    float transmissionFactor;     // 4 bytes (104-107)
    int hasRefractionMap;         // 4 bytes (108-111)
    
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

// Local reflection probes (up to 4 active probes)
struct ReflectionProbe {
    vec3 position;
    float intensity;
    vec3 boxMin;
    float blendDistance;
    vec3 boxMax;
    int isActive;
    vec4 influence; // xyz = size, w = priority
};

layout (std140) uniform ReflectionProbes {
    int activeProbeCount;
    ReflectionProbe probes[4];
};

// Individual probe cubemaps
uniform samplerCube reflectionProbe0;
uniform samplerCube reflectionProbe1;
uniform samplerCube reflectionProbe2;
uniform samplerCube reflectionProbe3;

// Shading mode toggle
uniform bool isBlinnPhong;

// View matrix for world space calculations
uniform mat4 view;
uniform mat4 model;

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
    
    if (material.hasNormalMap != 0) {
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
    vec3 albedo = material.albedo; // Use vec3 directly
    
    if (material.hasAlbedoMap != 0) {
        vec4 texColor = texture(materialAlbedoMap, TexCoord);
        albedo *= texColor.rgb;
    }
    
    return albedo;
}

float getRoughness()
{
    float roughness = material.roughness;
    if (material.hasRoughnessMap != 0) {
        roughness *= texture(materialRoughnessMap, TexCoord).r;
    }
    return clamp(roughness, 0.05, 1.0);
}

float getMetallic()
{
    float metallic = material.metallic;
    if (material.hasMetallicMap != 0) {
        metallic *= texture(materialMetallicMap, TexCoord).r;
    }
    return clamp(metallic, 0.0, 1.0);
}

float getAO()
{
    float ao = material.ao;
    if (material.hasAoMap != 0) {
        ao *= texture(materialAoMap, TexCoord).r;
    }
    return ao;
}

vec3 getEmissive()
{
    vec3 emissive = material.emissive * material.emissiveIntensity;
    if (material.hasEmissiveMap != 0) {
        vec4 emissiveTexel = texture(materialEmissiveMap, TexCoord);
        emissive *= emissiveTexel.rgb;
    }
    return emissive;
}

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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

// Box projection for local reflection probes
vec3 boxProjection(vec3 direction, vec3 position, vec3 boxMin, vec3 boxMax)
{
    vec3 rbmax = (boxMax - position) / direction;
    vec3 rbmin = (boxMin - position) / direction;
    
    vec3 rbminmax = max(rbmax, rbmin);
    float fa = min(min(rbminmax.x, rbminmax.y), rbminmax.z);
    
    vec3 worldPos = position + direction * fa;
    return worldPos;
}

// Calculate local reflection probe contribution
vec3 calculateLocalReflection(vec3 worldPos, vec3 normal, vec3 viewDir, float roughness)
{
    vec3 reflectionColor = vec3(0.0);
    float totalWeight = 0.0;
    
    for (int i = 0; i < activeProbeCount && i < 4; ++i) {
        ReflectionProbe probe = probes[i];
        if (probe.isActive == 0) continue;
        
        // Check if fragment is within probe influence
        vec3 localPos = worldPos - probe.position;
        if (all(greaterThan(localPos, probe.boxMin)) && all(lessThan(localPos, probe.boxMax))) {
            
            // Calculate blend weight based on distance to edge
            vec3 distToEdge = min(localPos - probe.boxMin, probe.boxMax - localPos);
            float minDist = min(min(distToEdge.x, distToEdge.y), distToEdge.z);
            float weight = clamp(minDist / probe.blendDistance, 0.0, 1.0);
            
            // Calculate reflection direction
            vec3 R = reflect(-viewDir, normal);
            
            // Apply box projection for more accurate local reflections
            vec3 projectedR = boxProjection(R, worldPos, 
                                          probe.position + probe.boxMin, 
                                          probe.position + probe.boxMax);
            vec3 correctedR = normalize(projectedR - probe.position);
            
            // Sample appropriate probe cubemap
            vec3 probeReflection;
            float mipLevel = roughness * 8.0; // Assuming 8 mip levels
            
            if (i == 0) probeReflection = textureLod(reflectionProbe0, correctedR, mipLevel).rgb;
            else if (i == 1) probeReflection = textureLod(reflectionProbe1, correctedR, mipLevel).rgb;
            else if (i == 2) probeReflection = textureLod(reflectionProbe2, correctedR, mipLevel).rgb;
            else if (i == 3) probeReflection = textureLod(reflectionProbe3, correctedR, mipLevel).rgb;
            
            // Apply probe intensity and weight
            reflectionColor += probeReflection * probe.intensity * weight;
            totalWeight += weight;
        }
    }
    
    // Fallback to global environment map if no local probes
    if (totalWeight < 0.001 && material.hasEnvironmentMap != 0) {
        mat3 viewToWorld = transpose(mat3(view));
        vec3 worldNormal = viewToWorld * normal;
        vec3 worldViewDir = viewToWorld * viewDir;
        
        vec3 R = reflect(-worldViewDir, worldNormal);
        float mipLevel = roughness * 8.0;
        reflectionColor = textureLod(materialEnvironmentMap, R, mipLevel).rgb * material.environmentIntensity;
        totalWeight = 1.0;
    }
    
    return reflectionColor / max(totalWeight, 0.001);
}

// Calculate environment refraction
vec3 calculateEnvironmentRefraction(vec3 normal, vec3 viewDir, float ior)
{
    if (material.hasRefractionMap == 0) return vec3(0.0);
    
    // Convert to world space for consistent environment mapping
    mat3 viewToWorld = transpose(mat3(view));
    vec3 worldNormal = viewToWorld * normal;
    vec3 worldViewDir = viewToWorld * viewDir;
    
    // Calculate refraction vector in world space
    vec3 refractionDir = refract(-worldViewDir, worldNormal, 1.0 / ior);
    
    // If total internal reflection occurs, fall back to reflection
    if (length(refractionDir) < 0.001) {
        refractionDir = reflect(-worldViewDir, worldNormal);
    }
    
    // Sample environment map for refraction with slight blur for realism
    vec3 envRefraction = vec3(0.0);
    if (material.hasEnvironmentMap != 0) {
        // Use slight mip bias for refracted rays to simulate scattering
        float mipLevel = 1.0; // Slightly blurred refraction
        envRefraction = textureLod(materialEnvironmentMap, refractionDir, mipLevel).rgb;
    } else {
        // Fallback color
        envRefraction = vec3(0.7, 0.9, 1.0); // Sky-like blue
    }
    
    return envRefraction * material.environmentIntensity;
}

// Enhanced environment reflection calculation
vec3 calculateEnvironmentReflection(vec3 normal, vec3 viewDir, float roughness)
{
    if (material.hasEnvironmentMap == 0) return vec3(0.0);
    
    // Convert to world space for consistent environment mapping
    mat3 viewToWorld = transpose(mat3(view));
    vec3 worldNormal = viewToWorld * normal;
    vec3 worldViewDir = viewToWorld * viewDir;
    
    // Calculate reflection vector in world space
    vec3 reflectionDir = reflect(-worldViewDir, worldNormal);
    
    // Use roughness to determine mip level for varying reflection sharpness
    float mipLevel = roughness * 8.0; // Assuming 8 mip levels
    vec3 envReflection = textureLod(materialEnvironmentMap, reflectionDir, mipLevel).rgb;
    
    return envReflection * material.environmentIntensity;
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
    
    // Calculate world position for local reflection probes
    vec3 worldPos = vec3(model * vec4(FragPos, 1.0));
    
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
        result += ambient;

        // Add contribution from each light
        for (int i = 0; i < numLights && i < 16; ++i) {
            result += calculateBlinnPhong(i, norm, viewDir, ViewPos, albedo);
        }
        
        // Add simple environment reflection for Blinn-Phong
        vec3 envReflection = calculateLocalReflection(worldPos, norm, viewDir, 0.2);
        result += envReflection * materialKs * 0.5;
        
        // Add emissive
        result += emissive;
        result += materialKe;
    } else {
        // PBR Lighting with advanced environment effects
        vec3 F0 = vec3(material.reflectance);
        F0 = mix(F0, albedo, metallic);
        
        // Ambient lighting from environment or fallback
        vec3 ambient = vec3(0.08) * albedo * ao;
        if (material.hasIrradianceMap != 0) {
            mat3 viewToWorld = transpose(mat3(view));
            vec3 worldNormal = viewToWorld * norm;
            
            vec3 kS = fresnelSchlickRoughness(max(dot(norm, viewDir), 0.0), F0, roughness);
            vec3 kD = 1.0 - kS;
            kD *= 1.0 - metallic;
            
            vec3 irradiance = texture(materialIrradianceMap, worldNormal).rgb;
            vec3 diffuse = irradiance * albedo;
            ambient = (kD * diffuse) * ao * material.environmentIntensity;
        }
        
        result += ambient;

        // Add contribution from each light
        for (int i = 0; i < numLights && i < 16; ++i) {
            result += calculatePBR(i, norm, viewDir, ViewPos, albedo, F0, roughness, metallic);
        }
        
        // Calculate Fresnel for reflection/refraction mixing
        float cosTheta = max(dot(norm, viewDir), 0.0);
        vec3 kS = fresnelSchlickRoughness(cosTheta, F0, roughness);
        float fresnel = kS.r; // Use red component as scalar
        
        // Environment reflections (local probes with fallback)
        vec3 envReflection = calculateLocalReflection(worldPos, norm, viewDir, roughness);
        
        // Environment refraction for transparent materials
        vec3 envRefraction = vec3(0.0);
        float transparency = material.transparency; // Use dedicated transparency field
        if (transparency > 0.0 && material.hasRefractionMap != 0) {
            envRefraction = calculateEnvironmentRefraction(norm, viewDir, material.indexOfRefraction);
        }
        
        // Enhanced Fresnel calculation for better mixing
        float fresnelFactor = (kS.r + kS.g + kS.b) / 3.0; // Average Fresnel as scalar
        
        // Mix reflection and refraction based on fresnel and material properties
        vec3 environmentContribution = vec3(0.0);
        if (transparency > 0.0) {
            // For transparent materials: blend reflection and refraction
            float reflectionStrength = fresnelFactor;
            float refractionStrength = (1.0 - fresnelFactor) * material.transmissionFactor * transparency;
            
            // Normalize to ensure energy conservation
            float totalStrength = reflectionStrength + refractionStrength;
            if (totalStrength > 0.0) {
                reflectionStrength /= totalStrength;
                refractionStrength /= totalStrength;
            }
            
            environmentContribution = envReflection * reflectionStrength + envRefraction * refractionStrength;
        } else {
            // For opaque materials: only reflection
            environmentContribution = envReflection * kS;
        }
        
        result += environmentContribution * ao;
        
        // Energy compensation for very rough surfaces
        if (roughness > 0.7) {
            result *= mix(1.0, 1.4, (roughness - 0.7) / 0.3);
        }
        
        // Add emissive
        result += emissive;
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
    
    // Use alpha from transparency field
    float alpha = 1.0 - material.transparency; // Convert transparency to alpha (1.0 = opaque, 0.0 = transparent)
    FragColor = vec4(result, alpha);
}