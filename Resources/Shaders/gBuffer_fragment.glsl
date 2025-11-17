#version 460 core
#extension GL_ARB_bindless_texture : require

// Input from vertex shader
in vec2 TexCoord;
in vec3 WorldPos;
in vec3 WorldNormal;
in vec3 ViewPos;
in vec3 ViewNormal;
in vec3 ViewTangent;
in vec3 ViewBitangent;
flat in uint vMaterialIndex; // Material index from vertex shader

// G-Buffer outputs
layout(location = 0) out vec3 gBuffer0; // RT0: Albedo
layout(location = 1) out vec3 gBuffer1; // RT1: Normal
layout(location = 2) out vec4 gBuffer2; // RT2: Emissive
layout(location = 3) out vec4 gBuffer3; // RT3: Material

// Material texture flag bits (must match C++ MaterialTextureFlags enum)
const uint MAT_FLAG_ALBEDO_MAP    = 1u << 0u;  // bit 0
const uint MAT_FLAG_NORMAL_MAP    = 1u << 1u;  // bit 1
const uint MAT_FLAG_ROUGHNESS_MAP = 1u << 2u;  // bit 2
const uint MAT_FLAG_METALLIC_MAP  = 1u << 3u;  // bit 3
const uint MAT_FLAG_AO_MAP        = 1u << 4u;  // bit 4
const uint MAT_FLAG_EMISSIVE_MAP  = 1u << 5u;  // bit 5

// Material structure
struct MaterialData {
    vec4 albedo;                    // 16 bytes (0-15)
    float metallic;                 // 4 bytes (16-19)
    float roughness;                // 4 bytes (20-23)
    float ao;                       // 4 bytes (24-27)
    float normalStrength;           // 4 bytes (28-31)

    vec3 emissive;                  // 12 bytes (32-43)
    float emissiveIntensity;        // 4 bytes (44-47)

    int shadingModel;               // 4 bytes (48-51)
    uint textureFlags;              // 4 bytes (52-55) - Packed bitfield for all texture flags
    int castsShadows;               // 4 bytes (56-59) - Whether this material casts shadows
    float _pad0;                    // 4 bytes (60-63)

    vec2 uvScale;                   // 8 bytes (64-71)
    vec2 uvOffset;                  // 8 bytes (72-79)

    // Texture Array Indices
    int albedoMapIndex;             // 4 bytes (80-83)
    int normalMapIndex;             // 4 bytes (84-87)
    int roughnessMapIndex;          // 4 bytes (88-91)
    int metallicMapIndex;           // 4 bytes (92-95)

    int aoMapIndex;                 // 4 bytes (96-99)
    int emissiveMapIndex;           // 4 bytes (100-103)
    int _pad2;                      // 4 bytes (104-107)
    int _pad3;                      // 4 bytes (108-111)
    // Total: 112 bytes (down from 128 bytes)
};

// Material SSBO - array of materials
layout(std430, binding = 3) restrict readonly buffer MaterialBlock
{
    MaterialData materials[];
};

// Bindless texture array SSBO - stores texture handles as uvec2 (64-bit split into two 32-bit values)
layout(std430, binding = 5) restrict readonly buffer TextureArrayBlock
{
    uvec2 textureHandles[];
};

// Material index uniform - which material to use from the array
uniform int u_MaterialIndex = 0;


// Pack albedo RGB into RT0 (RGB16F format)
vec3 packAlbedo(vec3 albedo) {
    // Direct storage - RGB16F provides sufficient precision for albedo
    return clamp(albedo, 0.0, 65504.0); // Clamp to half-float range
}

// Pack normal XYZ into RT1 (RGB16F format)
vec3 packNormal(vec3 normal) {
    // Normalize and convert from [-1,1] to [0,1] range for storage
    vec3 packedNormal = normalize(normal) * 0.5 + 0.5;
    return clamp(packedNormal, 0.0, 1.0);
}

// Pack emissive RGB + intensity into RT2 (RGBA8 format)
vec4 packEmissive(vec3 emissive, float emissiveIntensity) {
    if (emissiveIntensity < 0.001) {
        return vec4(0.0, 0.0, 0.0, 0.0); // Black emissive
    }
    
    float clampedIntensity = clamp(emissiveIntensity, 0.0, 255.0);
    
    return vec4(clamp(emissive, 0.0, 1.0), clampedIntensity / 255.0);
}

// Pack material properties into RT3 (RGBA8 format)
vec4 packMaterialProperties(float roughness, float metallic, float ao, float unused) {
    return vec4(
        clamp(roughness, 0.0, 1.0),
        clamp(metallic, 0.0, 1.0),
        clamp(ao, 0.0, 1.0),
        0.0 // Unused channel
    );
}

// Main G-Buffer output function (call this in geometry fragment shader)
void writeGBuffer(vec3 albedo, vec3 normal, vec3 emissive, float emissiveIntensity,
                  float roughness, float metallic, float ao) {
    // Output to multiple render targets
    gBuffer0 = packAlbedo(albedo);                                    // RT0: RGB16F
    gBuffer1 = packNormal(normal);                                    // RT1: RGB16F
    gBuffer2 = packEmissive(emissive, emissiveIntensity);            // RT2: RGBA8
    gBuffer3 = packMaterialProperties(roughness, metallic, ao, 0.0); // RT3: RGBA8
}

void main()
{
    // Get the material for this draw call from the array using the per-vertex material index
    MaterialData material = materials[vMaterialIndex];

    // Apply UV transform (scale and offset) - calculate once
    vec2 transformedUV = fma(TexCoord, material.uvScale, material.uvOffset);

    // Check if material uses any textures (early-out optimization for procedural materials)
    // Fast path: No textures, pure procedural material
    if (material.textureFlags == 0u) {
        writeGBuffer(material.albedo.rgb, ViewNormal, material.emissive,
                     material.emissiveIntensity, material.roughness,
                     material.metallic, material.ao);
        return;
    }

    // ========== BATCH TEXTURE SAMPLES (improves cache coherency) ==========
    // Sample all textures first to hide latency and improve texture cache usage
    // Use bitwise AND to check flags AND validate texture indices
    vec3 albedoSample = ((material.textureFlags & MAT_FLAG_ALBEDO_MAP) != 0u && material.albedoMapIndex >= 0)
        ? texture(sampler2D(textureHandles[material.albedoMapIndex]), transformedUV).rgb
        : vec3(1.0);

    vec3 normalSample = ((material.textureFlags & MAT_FLAG_NORMAL_MAP) != 0u && material.normalMapIndex >= 0)
        ? texture(sampler2D(textureHandles[material.normalMapIndex]), transformedUV).rgb
        : vec3(0.5, 0.5, 1.0);

    float roughnessSample = ((material.textureFlags & MAT_FLAG_ROUGHNESS_MAP) != 0u && material.roughnessMapIndex >= 0)
        ? texture(sampler2D(textureHandles[material.roughnessMapIndex]), transformedUV).r
        : 1.0;

    float metallicSample = ((material.textureFlags & MAT_FLAG_METALLIC_MAP) != 0u && material.metallicMapIndex >= 0)
        ? texture(sampler2D(textureHandles[material.metallicMapIndex]), transformedUV).r
        : 1.0;

    float aoSample = ((material.textureFlags & MAT_FLAG_AO_MAP) != 0u && material.aoMapIndex >= 0)
        ? texture(sampler2D(textureHandles[material.aoMapIndex]), transformedUV).r
        : 1.0;

    vec3 emissiveSample = ((material.textureFlags & MAT_FLAG_EMISSIVE_MAP) != 0u && material.emissiveMapIndex >= 0)
        ? texture(sampler2D(textureHandles[material.emissiveMapIndex]), transformedUV).rgb
        : vec3(1.0);

    // ========== PROCESS SAMPLES ==========
    // Albedo
    vec3 finalAlbedo = material.albedo.rgb * albedoSample;

    // Normal - optimized transformation
    vec3 finalNormal = ViewNormal;
    if ((material.textureFlags & MAT_FLAG_NORMAL_MAP) != 0u && material.normalMapIndex >= 0) {
        // Decode normal map using fma for efficiency
        vec3 tangentNormal = fma(normalSample, vec3(2.0), vec3(-1.0));
        tangentNormal.xy *= material.normalStrength;

        // Normalize only if normal strength modified the vector significantly
        if (abs(material.normalStrength - 1.0) > 0.01) {
            tangentNormal = normalize(tangentNormal);
        }

        // Transform to view space using explicit vector operations (faster than matrix multiply)
        finalNormal = normalize(ViewTangent * tangentNormal.x +
                               ViewBitangent * tangentNormal.y +
                               ViewNormal * tangentNormal.z);
    }

    // Material properties
    float finalRoughness = material.roughness * roughnessSample;
    float finalMetallic = material.metallic * metallicSample;
    float finalAO = material.ao * aoSample;

    // Emissive
    vec3 finalEmissive = material.emissive * emissiveSample;
    float finalEmissiveIntensity = material.emissiveIntensity;

    // Write to G-Buffer
    writeGBuffer(finalAlbedo, finalNormal, finalEmissive, finalEmissiveIntensity,
                 finalRoughness, finalMetallic, finalAO);
}