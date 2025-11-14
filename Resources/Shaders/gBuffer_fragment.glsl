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

// Material structure
struct MaterialData {
    vec4 albedo;                    // 16 bytes
    float metallic;                 // 4 bytes
    float roughness;                // 4 bytes
    float ao;                       // 4 bytes
    float normalStrength;           // 4 bytes

    vec3 emissive;                  // 12 bytes
    float emissiveIntensity;        // 4 bytes

    int shadingModel;               // 4 bytes (0 = PBR, 1 = Blinn-Phong)
    int hasAlbedoMap;               // 4 bytes
    int hasNormalMap;               // 4 bytes
    int hasRoughnessMap;            // 4 bytes

    int hasMetallicMap;             // 4 bytes
    int hasAoMap;                   // 4 bytes
    int hasEmissiveMap;             // 4 bytes
    float _pad0;                    // 4 bytes (padding)

    vec2 uvScale;                   // 8 bytes (UV scale)
    vec2 uvOffset;                  // 8 bytes (UV offset)

    // Texture Array Indices
    int albedoMapIndex;             // 4 bytes
    int normalMapIndex;             // 4 bytes
    int roughnessMapIndex;          // 4 bytes
    int metallicMapIndex;           // 4 bytes

    int aoMapIndex;                 // 4 bytes
    int emissiveMapIndex;           // 4 bytes
    int _pad1;                      // 4 bytes (padding)
    int _pad2;                      // 4 bytes (padding)
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

vec3 getNormalFromMap_TBN(sampler2D normalMap, vec2 texCoords, vec3 viewNormal, vec3 viewTangent, vec3 viewBitangent)
{
    // Sample normal map (tangent space normal)
    vec3 tangentNormal = texture(normalMap, texCoords).xyz * 2.0 - 1.0;
    
    // Construct TBN matrix using pre-calculated tangent and bitangent
    // Ensure all vectors are normalized and orthogonal
    vec3 T = normalize(viewTangent);
    vec3 B = normalize(viewBitangent);
    vec3 N = normalize(viewNormal);
    
    // Re-orthogonalize T with respect to N (Gram-Schmidt process)
    T = normalize(T - dot(T, N) * N);
    
    // Re-calculate B to ensure proper handedness
    B = cross(N, T);
    
    // Construct the TBN matrix
    mat3 TBN = mat3(T, B, N);
    
    // Transform tangent space normal to view space
    return normalize(TBN * tangentNormal);
}

void main()
{
    // Get the material for this draw call from the array using the per-vertex material index
    MaterialData material = materials[vMaterialIndex];

    // Apply UV transform (scale and offset)
    vec2 transformedUV = TexCoord * material.uvScale + material.uvOffset;

    // Sample material properties from textures if available
    vec3 finalAlbedo = material.albedo.rgb; // Use RGB components from vec4 albedo
    if (material.hasAlbedoMap != 0 && material.albedoMapIndex >= 0)
    {
        vec4 albedoSample = texture(sampler2D(textureHandles[material.albedoMapIndex]), transformedUV);
        finalAlbedo *= albedoSample.rgb;
    }

    // Normal mapping
    vec3 finalNormal = ViewNormal;
    if (material.hasNormalMap != 0 && material.normalMapIndex >= 0)
    {
        vec3 normalSample = texture(sampler2D(textureHandles[material.normalMapIndex]), transformedUV).rgb * 2.0 - 1.0;
        normalSample.xy *= material.normalStrength;

        mat3 TBN = mat3(normalize(ViewTangent), normalize(ViewBitangent), normalize(ViewNormal));
        finalNormal = normalize(TBN * normalSample);
    }

    // Material properties
    float finalRoughness = material.roughness;
    if (material.hasRoughnessMap != 0 && material.roughnessMapIndex >= 0)
    {
        finalRoughness *= texture(sampler2D(textureHandles[material.roughnessMapIndex]), transformedUV).r;
    }

    float finalMetallic = material.metallic;
    if (material.hasMetallicMap != 0 && material.metallicMapIndex >= 0)
    {
        finalMetallic *= texture(sampler2D(textureHandles[material.metallicMapIndex]), transformedUV).r;
    }

    float finalAO = material.ao;
    if (material.hasAoMap != 0 && material.aoMapIndex >= 0)
    {
        finalAO *= texture(sampler2D(textureHandles[material.aoMapIndex]), transformedUV).r;
    }

    vec3 finalEmissive = material.emissive;
    float finalEmissiveIntensity = material.emissiveIntensity;
    if (material.hasEmissiveMap != 0 && material.emissiveMapIndex >= 0)
    {
        vec3 emissiveSample = texture(sampler2D(textureHandles[material.emissiveMapIndex]), transformedUV).rgb;
        finalEmissive *= emissiveSample;
    }

    // Write to G-Buffer
    writeGBuffer(finalAlbedo, finalNormal, finalEmissive, finalEmissiveIntensity,
                 finalRoughness, finalMetallic, finalAO);
}