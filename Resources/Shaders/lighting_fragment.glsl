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
uniform mat4 projection;

// Shading mode
uniform int u_ShadingMode; // 0 = PBR, 1 = Blinn-Phong

// VBAO Parameters
uniform int u_VBAO = 1;
uniform int u_VBAOSlices = 4;
uniform int u_VBAOSteps = 16;
uniform float u_VBAORadius =  1.0;
uniform float u_VBAOThickness =  1.0; 
uniform float u_VBAOThicknessMultiplier = 0.2;
uniform float u_VBAOIntensity = 0.9;
uniform float u_VBAOFadeout = 0.9;
uniform float u_VBAOBias = 0.002;



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
const float HALF_PI = PI * 0.5;
const uint SECTOR_COUNT = 32u;
const int POINT_LIGHT = 0;
const int DIRECTIONAL_LIGHT = 1;
const int SPOT_LIGHT = 2;

// Helpers
uint fastBitCount(uint value) {
    // Brian Kernighan's algorithm
    value = value - ((value >> 1u) & 0x55555555u);
    value = (value & 0x33333333u) + ((value >> 2u) & 0x33333333u);
    return ((value + (value >> 4u) & 0xF0F0F0Fu) * 0x1010101u) >> 24u;
}

uint updateSectorBitmask(float minHorizon, float maxHorizon, uint existingMask) {
    // Convert normalized horizon angles to bit positions
    uint startBit = uint(clamp(minHorizon * float(SECTOR_COUNT), 0.0, 31.0));
    uint endBit = uint(clamp(maxHorizon * float(SECTOR_COUNT), 0.0, 31.0));
    
    if (endBit <= startBit) return existingMask;
    
    uint bitCount = endBit - startBit;
    uint mask = (bitCount >= 32u) ? 0xFFFFFFFFu : ((1u << bitCount) - 1u) << startBit;
    
    return existingMask | mask;
}

float bayer4x4(ivec2 coord) {
    const uint bayer[16] = uint[](
        0u, 8u, 2u, 10u,
        12u, 4u, 14u, 6u,
        3u, 11u, 1u, 9u,
        15u, 7u, 13u, 5u
    );
    return float(bayer[(coord.x & 3) + (coord.y & 3) * 4]) * (1.0 / 16.0);
}

float interleavedGradientNoise(vec2 coord) {
    // IGN - single multiply-add chain
    return fract(52.9829189 * fract(0.06711056 * coord.x + 0.00583715 * coord.y));
}

vec3 getViewPosition(vec2 texCoord, float depth) {
    vec4 ndc = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = invProjection * ndc;
    return viewPos.xyz / viewPos.w;
}

vec2 fastAcos2(vec2 x) {
    return (-0.69813170 * x * x - 0.87266463) * x + 1.57079633;
}

float fastDotNormalized(vec3 a, vec3 b) {
    return dot(a, b) * inversesqrt(dot(a, a));
}

vec2 traceSliceBitmaskOptimized(vec2 texCoord, vec3 viewPos, vec3 viewDir, vec3 normal,
                               vec2 sliceDir, float jitter, inout uint bitfield,
                               float dirSign, float N, vec3 projectedNormal) {
    
    vec2 texelSize = 1.0 / textureSize(sampler2D(u_GBufferDepthHandle), 0);
    vec2 rayOffset = 1.4 * dirSign * sliceDir * texelSize;
    vec2 scaledDir = sliceDir * vec2(1.0, texelSize.x / texelSize.y);
    
    float h = dirSign * sin(N);
    
    // Pre-calculate constants outside loop
    const float radiusScale = 0.1 * u_VBAORadius;
    const float invSteps = 1.0 / float(u_VBAOSteps);
    
    for(int i = 0; i < u_VBAOSteps; i++) {
        float stepRatio = (float(i) + jitter) * invSteps;
        stepRatio = stepRatio * stepRatio * stepRatio; // Cubic distribution for better near sampling
        
        vec2 sampleCoord = texCoord + rayOffset + dirSign * radiusScale * scaledDir * stepRatio;
        
        // Optimized depth sampling - use texelFetch for near samples (faster)
        float sampleDepth;
        if(stepRatio < 0.7) {
            ivec2 iCoord = ivec2(sampleCoord * textureSize(sampler2D(u_GBufferDepthHandle), 0));
            sampleDepth = texelFetch(sampler2D(u_GBufferDepthHandle), iCoord, 0).r;
        } else {
            sampleDepth = texture(sampler2D(u_GBufferDepthHandle), sampleCoord).r;
        }
        
        vec3 samplePos = getViewPosition(sampleCoord, sampleDepth);
        vec3 toSample = samplePos - viewPos;
        
        float sampleDotView = fastDotNormalized(toSample, viewDir);
        
        // Apply user-adjustable thickness with optimized calculation
        vec3 thicknessSample = normalize(samplePos) * u_VBAOThickness + 
                              (1.0 + u_VBAOThicknessMultiplier) * samplePos - viewPos;
        float thicknessDotView = fastDotNormalized(thicknessSample, viewDir);
        
        vec2 angles = fastAcos2(vec2(sampleDotView, thicknessDotView));
        
        // Distance-based attenuation (optimized)
        float distSq = dot(toSample, toSample);
        float attenuation = 1.0 / (0.01 * distSq / dot(samplePos, samplePos) + 1.0);
        h = mix(h, max(h, sampleDotView), mix(1.0, stepRatio, 0.75) * attenuation);
        
        // Convert angles to normalized bitmask coordinates (optimized)
        vec2 normalizedAngles = clamp((dirSign * -angles - N + HALF_PI) / PI, 0.0, 1.0);
        normalizedAngles = normalizedAngles.x > normalizedAngles.y ? normalizedAngles.yx : normalizedAngles;
        
        // Fast bit operations - update bitmask
        bitfield = updateSectorBitmask(normalizedAngles.x, normalizedAngles.y, bitfield);
    }
    
    return vec2(h, 0.0);
}

float calculateVBAOOptimized(vec2 texCoord, vec3 viewPos, vec3 viewDir, vec3 normal, vec2 noise) {
    float totalAO = 0.0;
    float totalWeight = 0.0;
    
    // Pre-calculate slice rotation increment
    const float sliceRotation = PI / float(u_VBAOSlices);
    
    for(int slice = 0; slice < u_VBAOSlices; slice++) {
        float angle = (float(slice) + noise.x) * sliceRotation;
        vec2 sliceDir = vec2(sin(angle), cos(angle));
        
        vec3 sliceNormal = normalize(cross(vec3(sliceDir, 0.0), viewDir));
        vec3 tangent = cross(viewDir, sliceNormal);
        
        vec3 projectedNormal = normal - sliceNormal * dot(normal, sliceNormal);
        float projectedLength = length(projectedNormal);
        
        if(projectedLength < 0.01) continue; // Skip perpendicular slices
        
        float N = -sign(dot(projectedNormal, tangent)) * 
                  acos(clamp(dot(normalize(projectedNormal), viewDir), -1.0, 1.0));
        
        vec3 normalizedProjected = normalize(projectedNormal);
        
        // Initialize bitmask for this slice
        uint bitfield = 0u;
        vec4 horizons;
        
        // Trace both directions of the slice
        horizons.xz = traceSliceBitmaskOptimized(texCoord, viewPos, viewDir, normal, sliceDir, 
                                                noise.y, bitfield, 1.0, N, normalizedProjected);
        horizons.yw = traceSliceBitmaskOptimized(texCoord, viewPos, viewDir, normal, sliceDir, 
                                                noise.y, bitfield, -1.0, N, normalizedProjected);
        
        // Calculate visibility from bitmask using optimized bit count
        float visibility = 1.0 - float(fastBitCount(bitfield)) / float(SECTOR_COUNT);
        
        // Weight by projected normal length (slice importance)
        float sliceWeight = projectedLength;
        totalAO += visibility * sliceWeight;
        totalWeight += sliceWeight;
    }
    
    return totalWeight > 0.0 ? totalAO / totalWeight : 1.0;
}

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

vec3 reconstructViewPosition(vec2 texCoord, float depth) {
    vec4 ndc = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = invProjection * ndc;
    return viewPos.xyz / viewPos.w;
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
                 vec3 albedo, float metallic, float roughness, vec3 F0, vec3 fragPosWorld)
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
    emissive = packedEmissive.rgb;
    emissiveIntensity = packedEmissive.a * 255.0;
    
    // Check for no emissive contribution
    if (emissiveIntensity < 0.001 || length(emissive) < 0.001) {
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

    
    // Apply bias to prevent self-occlusion
    vec3 biasedViewPos = fragPosView + u_VBAOBias * normalView * length(fragPosView);
    
    // Calculate lighting using your existing system
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
        
        // Calculate and apply VBAO
        if(u_VBAO == 1) {
            // Generate optimized temporal noise
            ivec2 pixelCoord = ivec2(TexCoord * textureSize(sampler2D(u_GBufferDepthHandle), 0));
            float bayerNoise = bayer4x4(pixelCoord);
            float gradientNoise = interleavedGradientNoise(TexCoord);
            vec2 noise = vec2(bayerNoise, gradientNoise);
            
            float aoFactor = calculateVBAOOptimized(TexCoord, biasedViewPos, viewDir, normalView, noise);
            
            // Apply intensity and distance fadeout
            aoFactor = mix(1.0, aoFactor, u_VBAOIntensity);
            aoFactor = mix(1.0, aoFactor, exp(-2.0 * (1.0 - u_VBAOFadeout) * depth));
            
            // Apply AO to lighting (multiply by AO factor)
            result *= aoFactor;
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
            result += calculatePBR(i, normalView, viewDir, fragPosView, albedo, metallic, roughness, F0, worldPos);
        }

        // Energy compensation for rough surfaces
        if (roughness > 0.7) {
            result *= mix(1.0, 1.4, (roughness - 0.7) / 0.3);
        }

        // Calculate and apply VBAO
        if(u_VBAO == 1) {
            // Generate optimized temporal noise
            ivec2 pixelCoord = ivec2(TexCoord * textureSize(sampler2D(u_GBufferDepthHandle), 0));
            float bayerNoise = bayer4x4(pixelCoord);
            float gradientNoise = interleavedGradientNoise(TexCoord);
            vec2 noise = vec2(bayerNoise, gradientNoise);
            
            float aoFactor = calculateVBAOOptimized(TexCoord, biasedViewPos, viewDir, normalView, noise);
            
            // Apply intensity and distance fadeout
            aoFactor = mix(1.0, aoFactor, u_VBAOIntensity);
            aoFactor = mix(1.0, aoFactor, exp(-2.0 * (1.0 - u_VBAOFadeout) * depth));
            
            // Apply AO to lighting (multiply by AO factor)
            result *= aoFactor;
        }

        // Emissive
        result += emissive * emissiveIntensity;
    }

    FragColor = vec4(result, 1.0);
}