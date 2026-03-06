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
flat in uint vDrawFlags;     // DrawInfo flags from vertex shader

// G-Buffer outputs
// RT0: RGBA8  - Albedo RGB (albedo is always [0,1], 8bpc sufficient, saves 16 bits vs RGB16F)
// RT1: RG16F  - Normal oct-encoded (unit vectors have 2 DOF, saves 16 bits vs RGB16F)
// RT2: RGBA8  - Emissive RGB + Intensity
// RT3: RGBA8  - Roughness, Metallic, AO, (A free)
// RT4: RG16F  - Screen-space velocity XY (per-pixel, written from vertex shader)
layout(location = 0) out vec4 gBuffer0;
layout(location = 1) out vec2 gBuffer1;
layout(location = 2) out vec4 gBuffer2;
layout(location = 3) out vec4 gBuffer3;
layout(location = 4) out vec2 gBuffer4;

// Material texture flag bits (must match C++ MaterialTextureFlags enum)
const uint MAT_FLAG_ALBEDO_MAP    = 1u << 0u;  // bit 0
const uint MAT_FLAG_NORMAL_MAP    = 1u << 1u;  // bit 1
const uint MAT_FLAG_ROUGHNESS_MAP = 1u << 2u;  // bit 2
const uint MAT_FLAG_METALLIC_MAP  = 1u << 3u;  // bit 3
const uint MAT_FLAG_AO_MAP        = 1u << 4u;  // bit 4
const uint MAT_FLAG_EMISSIVE_MAP  = 1u << 5u;  // bit 5
const uint FLAG_CAMERA_ATTACHED   = 1u << 1u;  // DrawInfo flag bit 1

// Bindless texture array SSBO - stores texture handles as uvec2 (64-bit split into two 32-bit values)
layout(std430, binding = 5) restrict readonly buffer TextureArrayBlock
{
    uvec2 textureHandles[];
};

// ========== OCTAHEDRAL NORMAL ENCODING ==========
// Packs a unit normal into two [0,1] values. Quality comparable to RGB16F at half the storage.
vec2 signNotZero(vec2 v) {
    return vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

vec2 octEncode(vec3 n) {
    float l1norm = abs(n.x) + abs(n.y) + abs(n.z);
    vec2 p = n.xy / l1norm;
    if (n.z <= 0.0) p = (1.0 - abs(p.yx)) * signNotZero(p);
    return p * 0.5 + 0.5;  // remap [-1,1] -> [0,1] for RG16F storage
}

// Pack albedo RGB into RT0 (RGBA8 format, albedo is always [0,1])
vec4 packAlbedo(vec3 albedo) {
    return vec4(clamp(albedo, 0.0, 1.0), 1.0);
}

// Pack normal into RT1 (RG16F format, oct-encoded)
vec2 packNormal(vec3 normal) {
    return octEncode(normalize(normal));
}

// Pack emissive RGB + intensity into RT2 (RGBA8 format)
vec4 packEmissive(vec3 emissive, float emissiveIntensity) {
    if (emissiveIntensity < 0.001) {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
    return vec4(clamp(emissive, 0.0, 1.0), clamp(emissiveIntensity, 0.0, 255.0) / 255.0);
}

// Pack material properties into RT3 (RGBA8 format)
vec4 packMaterialProperties(float roughness, float metallic, float ao) {
    return vec4(
        clamp(roughness, 0.0, 1.0),
        clamp(metallic, 0.0, 1.0),
        clamp(ao, 0.0, 1.0),
        1.0  // alpha free for future use
    );
}

void writeGBuffer(vec3 albedo, vec3 normal, vec3 emissive, float emissiveIntensity,
                  float roughness, float metallic, float ao, vec2 velocity) {
    gBuffer0 = packAlbedo(albedo);
    gBuffer1 = packNormal(normal);
    gBuffer2 = packEmissive(emissive, emissiveIntensity);
    gBuffer3 = packMaterialProperties(roughness, metallic, ao);
    gBuffer4 = velocity;
}

// ========== MATERIAL DATA PASSED FROM VERTEX SHADER ==========
flat in vec4 vAlbedo;              // rgb=albedo, a=metallic
flat in vec4 vRoughnessAoEmissive; // r=roughness, g=ao, b=emissiveIntensity, a=normalStrength
flat in vec3 vEmissive;
flat in uint vTextureFlags;
flat in ivec4 vTextureIndices;     // albedo, normal, roughness, metallic
flat in ivec2 vTextureIndices2;    // ao, emissive

in vec2 vTransformedUV;   // UV with scale/offset already applied
in vec4 vCurrClipPos; // Current clip-space position
in vec4 vPrevClipPos; // Previous clip-space position

vec2 computeVelocity()
{
    if ((vDrawFlags & FLAG_CAMERA_ATTACHED) != 0u) {
        return vec2(0.0);
    }

    const float EPS = 1e-6;
    if (abs(vCurrClipPos.w) < EPS || abs(vPrevClipPos.w) < EPS) {
        return vec2(0.0);
    }

    vec2 currNDC = vCurrClipPos.xy / vCurrClipPos.w;
    vec2 prevNDC = vPrevClipPos.xy / vPrevClipPos.w;
    return (currNDC - prevNDC) * vec2(0.5, 0.5);
}

void main()
{
    // Unpack material data from varyings (NO SSBO ACCESS!)
    vec3 albedo = vAlbedo.rgb;
    float metallic = vAlbedo.a;
    float roughness = vRoughnessAoEmissive.r;
    float ao = vRoughnessAoEmissive.g;
    float emissiveIntensity = vRoughnessAoEmissive.b;
    float normalStrength = vRoughnessAoEmissive.a;

    // Fast path: No textures, pure procedural material
    if (vTextureFlags == 0u) {
        writeGBuffer(albedo, ViewNormal, vEmissive,
                     emissiveIntensity, roughness,
                     metallic, ao, computeVelocity());
        return;
    }

    // ========== BATCH TEXTURE SAMPLES ==========
    vec3 albedoSample = ((vTextureFlags & MAT_FLAG_ALBEDO_MAP) != 0u && vTextureIndices.x >= 0)
        ? texture(sampler2D(textureHandles[vTextureIndices.x]), vTransformedUV).rgb
        : vec3(1.0);

    vec3 normalSample = ((vTextureFlags & MAT_FLAG_NORMAL_MAP) != 0u && vTextureIndices.y >= 0)
        ? texture(sampler2D(textureHandles[vTextureIndices.y]), vTransformedUV).rgb
        : vec3(0.5, 0.5, 1.0);

    float roughnessSample = ((vTextureFlags & MAT_FLAG_ROUGHNESS_MAP) != 0u && vTextureIndices.z >= 0)
        ? texture(sampler2D(textureHandles[vTextureIndices.z]), vTransformedUV).r
        : 1.0;

    float metallicSample = ((vTextureFlags & MAT_FLAG_METALLIC_MAP) != 0u && vTextureIndices.w >= 0)
        ? texture(sampler2D(textureHandles[vTextureIndices.w]), vTransformedUV).r
        : 1.0;

    float aoSample = ((vTextureFlags & MAT_FLAG_AO_MAP) != 0u && vTextureIndices2.x >= 0)
        ? texture(sampler2D(textureHandles[vTextureIndices2.x]), vTransformedUV).r
        : 1.0;

    vec3 emissiveSample = ((vTextureFlags & MAT_FLAG_EMISSIVE_MAP) != 0u && vTextureIndices2.y >= 0)
        ? texture(sampler2D(textureHandles[vTextureIndices2.y]), vTransformedUV).rgb
        : vec3(1.0);

    // ========== PROCESS SAMPLES ==========
    vec3 finalAlbedo = albedo * albedoSample;

    vec3 finalNormal = ViewNormal;
    if ((vTextureFlags & MAT_FLAG_NORMAL_MAP) != 0u && vTextureIndices.y >= 0) {
        vec3 tangentNormal = fma(normalSample, vec3(2.0), vec3(-1.0));
        tangentNormal.xy *= normalStrength;

        if (abs(normalStrength - 1.0) > 0.01) {
            tangentNormal = normalize(tangentNormal);
        }

        finalNormal = normalize(ViewTangent * tangentNormal.x +
                               ViewBitangent * tangentNormal.y +
                               ViewNormal * tangentNormal.z);
    }

    float finalRoughness = roughness * roughnessSample;
    float finalMetallic = metallic * metallicSample;
    float finalAO = ao * aoSample;
    vec3 finalEmissive = vEmissive * emissiveSample;

    writeGBuffer(finalAlbedo, finalNormal, finalEmissive, emissiveIntensity,
                 finalRoughness, finalMetallic, finalAO, computeVelocity());
}
