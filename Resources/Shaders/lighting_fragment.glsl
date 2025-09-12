#version 460 core
#extension GL_ARB_bindless_texture : require


in vec2 TexCoord;
out vec4 FragColor;

// Bindless texture handles
uniform uvec2 u_GBuffer0Handle;
uniform uvec2 u_GBuffer1Handle;
uniform uvec2 u_GBuffer2Handle;
uniform uvec2 u_GBuffer3Handle;
uniform uvec2 u_GBufferDepthHandle; 

// Matrices for position reconstruction
uniform mat4 view;
uniform mat4 invView;      
uniform mat4 invProjection;   

// Shading mode
uniform int u_ShadingMode; // 0 = PBR, 1 = Blinn-Phong

// Light structure
struct Light {
    vec4 position_type;    // xyz = position (view space), w = light type
    vec4 color_intensity;  // xyz = color, w = intensity
    vec4 direction_range;  // xyz = direction (view space), w = range
    vec4 spot_angles;      // x = inner cos, y = outer cos
};

layout (std140) uniform Lights {
    vec4 lightCount;       // x = count, yzw unused
    Light lights[16];
};

// Constants
const float PI = 3.14159265359;
const int POINT_LIGHT = 0;
const int DIRECTIONAL_LIGHT = 1;
const int SPOT_LIGHT = 2;

// Reconstruct world position from depth
vec3 reconstructWorldPosition(vec2 texCoord, float depth)
{
    // Convert to NDC
    vec4 ndc = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    
    // Transform to view space
    vec4 viewPos = invProjection * ndc;
    viewPos /= viewPos.w;
    
    // Transform to world space
    vec4 worldPos = invView * viewPos;
    
    return worldPos.xyz;
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

// Calculate light attenuation and spot effect
float calculateAttenuation(int lightIndex, vec3 fragPosView, out vec3 lightDir)
{
    int lightType = int(lights[lightIndex].position_type.w);
    vec3 lightPosView = lights[lightIndex].position_type.xyz;
    float range = lights[lightIndex].direction_range.w;
    
    float attenuation = 1.0;
    
    if (lightType == DIRECTIONAL_LIGHT) {
        // Direction is stored directly in view space
        lightDir = normalize(-lights[lightIndex].direction_range.xyz);
        attenuation = 1.0;
    } else {
        // Point or spot: direction from light to fragment
        lightDir = normalize(lightPosView - fragPosView);
        float distance = length(lightPosView - fragPosView);
        
        // Attenuation
        float linearTerm = 0.045;
        float quadraticTerm = 0.0075;
        attenuation = 1.0 / (1.0 + linearTerm * distance + quadraticTerm * distance * distance);
        
        // Range fade
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
        
        // Spot cone
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

// Blinn-Phong shading
vec3 calculateBlinnPhong(int lightIndex, vec3 normal, vec3 viewDir, vec3 fragPosView, 
                        vec3 albedo, float ksIntensity, float shininess)
{
    vec3 lightDir;
    float attenuation = calculateAttenuation(lightIndex, fragPosView, lightDir);
    
    if (attenuation <= 0.0) return vec3(0.0);
    
    vec3 lightColor = lights[lightIndex].color_intensity.xyz * lights[lightIndex].color_intensity.w;
    
    vec3 materialKd = albedo;
    vec3 materialKs = vec3(ksIntensity);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * materialKd;
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = (shininess <= 0.0) ? 0.0 : pow(max(dot(normal, halfwayDir), 0.0), shininess * 256.0);
    vec3 specular = spec * lightColor * materialKs;
    
    return (diffuse + specular) * attenuation;
}

// PBR shading
vec3 calculatePBR(int lightIndex, vec3 normal, vec3 viewDir, vec3 fragPosView, 
                 vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3 lightDir;
    float attenuation = calculateAttenuation(lightIndex, fragPosView, lightDir);
    
    if (attenuation <= 0.0) return vec3(0.0);
    
    vec3 lightColor = lights[lightIndex].color_intensity.xyz * lights[lightIndex].color_intensity.w;
    vec3 radiance = lightColor * attenuation;
    
    vec3 H = normalize(viewDir + lightDir);
    roughness = clamp(roughness, 0.05, 1.0);
    
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

// Unpack albedo from RT0 (RGB16F format)
vec3 unpackAlbedo(vec3 packedAlbedo) {
    // Direct read - RGB16F stores albedo directly
    return packedAlbedo;
}

// Unpack normal from RT1 (RGB16F format)
vec3 unpackNormal(vec3 packedNormal) {
    // Convert from [0,1] back to [-1,1] range and normalize
    vec3 normal = packedNormal * 2.0 - 1.0;
    return normalize(normal);
}

// Unpack emissive from RT2 (RGBA8 format)
void unpackEmissive(vec4 packedEmissive, out vec3 emissive, out float emissiveIntensity) {
    vec3 normalizedRGB = packedEmissive.rgb;
    float exponent = packedEmissive.a * 255.0;
    
    if (exponent < 1.0) {
        emissive = vec3(0.0);
        emissiveIntensity = 0.0;
        return;
    }
    
    // Reconstruct scale factor
    float scale = exp2(exponent - 128.0);
    
    // Reconstruct emissive color and intensity
    vec3 scaledEmissive = normalizedRGB * scale;
    float totalIntensity = length(scaledEmissive);
    
    if (totalIntensity > 0.0) {
        emissive = scaledEmissive / totalIntensity;
        emissiveIntensity = totalIntensity;
    } else {
        emissive = vec3(0.0);
        emissiveIntensity = 0.0;
    }
}

// Unpack material properties from RT3 (RGBA8 format)
void unpackMaterialProperties(vec4 packedMaterial, out float roughness, 
                             out float metallic, out float ao) {
    roughness = packedMaterial.r;
    metallic = packedMaterial.g;
    ao = packedMaterial.b;
    // packedMaterial.a is unused
}

// Main G-Buffer reading function (call this in lighting fragment shader)
void readGBuffer(sampler2D gBuffer0, sampler2D gBuffer1, sampler2D gBuffer2, sampler2D gBuffer3,
                 vec2 texCoords, out vec3 albedo, out vec3 normal, out vec3 emissive, 
                 out float emissiveIntensity, out float roughness, out float metallic, out float ao) {
    
    // Sample all G-Buffer textures
    vec3 packedAlbedo = texture(gBuffer0, texCoords).rgb;
    vec3 packedNormal = texture(gBuffer1, texCoords).rgb;
    vec4 packedEmissive = texture(gBuffer2, texCoords);
    vec4 packedMaterial = texture(gBuffer3, texCoords);
    
    // Unpack all components
    albedo = unpackAlbedo(packedAlbedo);
    normal = unpackNormal(packedNormal);
    unpackEmissive(packedEmissive, emissive, emissiveIntensity);
    unpackMaterialProperties(packedMaterial, roughness, metallic, ao);
}

void readGBufferBindless(uvec2 gBuffer0Handle, uvec2 gBuffer1Handle, uvec2 gBuffer2Handle, uvec2 gBuffer3Handle,
                        vec2 texCoords, out vec3 albedo, out vec3 normal, out vec3 emissive,
                        out float emissiveIntensity, out float roughness, out float metallic, out float ao) {
    
    // Convert handles to samplers
    sampler2D gBuffer0 = sampler2D(gBuffer0Handle);
    sampler2D gBuffer1 = sampler2D(gBuffer1Handle);
    sampler2D gBuffer2 = sampler2D(gBuffer2Handle);
    sampler2D gBuffer3 = sampler2D(gBuffer3Handle);
    
    readGBuffer(gBuffer0, gBuffer1, gBuffer2, gBuffer3, texCoords, 
                albedo, normal, emissive, emissiveIntensity, roughness, metallic, ao);
}

void main()
{    
    // Smaple depth
    sampler2D gBufferDepthSampler = sampler2D(u_GBufferDepthHandle);
    float depth = texture(gBufferDepthSampler, TexCoord).r;

    // Early exit for background pixels
    if (depth >= 1.0) {
        FragColor = vec4(0.2f,0.3f,0.3f,1.0f);
        return;
    }
    
    // Unpack G-Buffer data
    vec3 albedo, normalView, emissive;
    float emissiveIntensity, metallic, roughness, ao;

    readGBufferBindless(u_GBuffer0Handle, u_GBuffer1Handle, u_GBuffer2Handle, u_GBuffer3Handle,
                       TexCoord, albedo, normalView, emissive, emissiveIntensity, 
                       roughness, metallic, ao);

    // Reconstruct world position
    vec3 worldPos = reconstructWorldPosition(TexCoord, depth);

    // Convert to view space for lighting calculations
    vec4 viewPos4 = view * vec4(worldPos, 1.0);
    vec3 fragPosView = viewPos4.xyz / viewPos4.w;

    // Normalze
    normalView = normalize(normalView);

    // View direction in view space (towards camera)
    vec3 viewDir = normalize(-fragPosView);

    int numLights = int(lightCount.x);

    // Shading model selection
    bool useBlinnPhong = (u_ShadingMode == 1);

    vec3 result = vec3(0.0);

    if (useBlinnPhong) {
        // Ambient
        vec3 ambient = vec3(0.2) * 0.1 * albedo * ao;
        result += ambient;

        // Blinn-Phong lighting
        for (int i = 0; i < numLights && i < 16; ++i) {
            float shininess = (1.0 - roughness) * 128.0;
            result += calculateBlinnPhong(i, normalView, viewDir, fragPosView, albedo, 1.0, shininess);
        }

        // Emissive
        result += emissive * emissiveIntensity;
    } else {
        // PBR ambient
        vec3 ambient = vec3(0.08) * albedo * ao;
        result += ambient;

        // PBR lighting
        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        for (int i = 0; i < numLights && i < 16; ++i) {
            result += calculatePBR(i, normalView, viewDir, fragPosView, albedo, metallic, roughness, F0);
        }

        // Energy compensation for rough surfaces
        if (roughness > 0.7) {
            result *= mix(1.0, 1.4, (roughness - 0.7) / 0.3);
        }

        // Emissive
        result += emissive * emissiveIntensity;
    }

    // Tone mapping (ACES approximation)
    vec3 a = 2.51 * result;
    vec3 b = 0.03 + result;
    vec3 c = 2.43 * result + 0.59;
    vec3 d = 0.14 + result;
    result = clamp((a * b) / (c * d), 0.0, 1.0);

    // Gamma correction
    result = pow(result, vec3(1.0/2.2));

    FragColor = vec4(result, 1.0);
}