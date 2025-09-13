#version 460 core

// Input from vertex shader
in vec2 TexCoord;
in vec3 WorldPos;
in vec3 WorldNormal;
in vec3 ViewPos;
in vec3 ViewNormal;

// G-Buffer outputs - matches your optimized format
layout(location = 0) out uvec3 gBuffer0; // RT0: RGB32_UINT (Albedo + Normal + Emissive)
layout(location = 1) out uvec2 gBuffer1; // RT1: RG32_UINT (Material + Motion vectors)

// Material UBO - matches your MaterialUBO structure
layout(std140) uniform MaterialBlock
{
    vec3 albedo;                    // 16-byte aligned
    float metallic;                 // 4 bytes
    float roughness;                // 4 bytes  
    float ao;                       // 4 bytes
    vec3 emissive;                  // 16-byte aligned
    float emissiveIntensity;        // 4 bytes
    float normalStrength;           // 4 bytes
    int shadingModel;               // 4 bytes (0 = PBR, 1 = Blinn-Phong)
    
    // Texture presence flags - MUST be int to match C++ MaterialUBO
    int hasAlbedoMap;               // 4 bytes
    int hasNormalMap;               // 4 bytes
    int hasRoughnessMap;            // 4 bytes
    int hasMetallicMap;             // 4 bytes
    int hasAoMap;                   // 4 bytes
    int hasEmissiveMap;             // 4 bytes
    
    int padding1;                   // 4 bytes
    int padding2;                   // 4 bytes
};

// Texture Samplers
uniform sampler2D materialAlbedoMap;
uniform sampler2D materialNormalMap;
uniform sampler2D materialRoughnessMap;
uniform sampler2D materialMetallicMap;
uniform sampler2D materialAoMap;
uniform sampler2D materialEmissiveMap;


// Packing functions based on your renderer specifications
uint packAlbedoShadingModel(vec3 albedo, bool isBlinnPhong)
{
    // Pack RGB 9:9:9 + ShadingModel 1-bit + 4 spare bits
    uvec3 rgb = uvec3(clamp(albedo * 511.0, 0.0, 511.0));
    uint sm = isBlinnPhong ? 1u : 0u;
    return (sm << 27) | (rgb.b << 18) | (rgb.g << 9) | rgb.r;
}

uint packNormal(vec3 normal)
{
    // Pack RGB 11:10:11 format
    vec3 n = (normal + 1.0) * 0.5; // Convert from [-1,1] to [0,1]
    uvec3 xyz = uvec3(
        clamp(n.x * 2047.0, 0.0, 2047.0), // 11 bits
        clamp(n.y * 1023.0, 0.0, 1023.0), // 10 bits  
        clamp(n.z * 2047.0, 0.0, 2047.0)  // 11 bits
    );
    return (xyz.z << 21) | (xyz.y << 11) | xyz.x;
}
uint packEmissive(vec3 emissive, float emissiveIntensity)
{
    // Pack RGBE 9:9:9:5 format
    vec3 scaledEmissive = emissive * emissiveIntensity;
    float maxComponent = max(scaledEmissive.x, max(scaledEmissive.y, scaledEmissive.z));
    
    if (maxComponent < 1e-32) return 0u; // Black emissive
    
    int exponent = int(floor(log2(maxComponent))) + 15; // Bias by 15
    exponent = clamp(exponent, 0, 31); // 5-bit range
    
    // Scale values to fit in 9-bit mantissa range [0, 511]
    float scale = exp2(float(exponent - 15));
    uvec3 rgb = uvec3(clamp(scaledEmissive / scale * 511.0, 0.0, 511.0));
    
    return (uint(exponent) << 27) | (rgb.b << 18) | (rgb.g << 9) | rgb.r;
}

uint packMaterialProperties(float metallic, float roughness, float ao, float normalStrength)
{
    // Pack Metallic 8-bits + Roughness 8-bits + AO 8-bits + NormalStrength 8-bits
    uvec4 props = uvec4(
        clamp(metallic * 255.0, 0.0, 255.0),
        clamp(roughness * 255.0, 0.0, 255.0),
        clamp(ao * 255.0, 0.0, 255.0),
        clamp(normalStrength * 255.0, 0.0, 255.0)
    );
    return (props.w << 24) | (props.z << 16) | (props.y << 8) | props.x;
}

vec3 getNormalFromMap(sampler2D normalMap, vec2 texCoords, vec3 worldNormal, vec3 worldPos)
{
    // Sample normal map
    vec3 tangentNormal = texture(normalMap, texCoords).rgb * 2.0 - 1.0;
    
    // Create TBN matrix
    vec3 Q1 = dFdx(worldPos);
    vec3 Q2 = dFdy(worldPos);
    vec2 st1 = dFdx(texCoords);
    vec2 st2 = dFdy(texCoords);
    
    vec3 N = normalize(worldNormal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

void main()
{
    // Sample material properties from textures if available
    vec3 finalAlbedo = albedo;
    if (hasAlbedoMap != 0)
    {
        vec4 albedoSample = texture(materialAlbedoMap, TexCoord);
        finalAlbedo *= albedoSample.rgb;
    }
    
    vec3 finalNormal = WorldNormal;
    if (hasNormalMap != 0)
    {
        finalNormal = getNormalFromMap(materialNormalMap, TexCoord, WorldNormal, WorldPos);
        // Apply normal strength
        finalNormal = normalize(mix(WorldNormal, finalNormal, normalStrength));
    }
    
    float finalRoughness = roughness;
    if (hasRoughnessMap != 0)
    {
        finalRoughness *= texture(materialRoughnessMap, TexCoord).r;
    }
    
    float finalMetallic = metallic;
    if (hasMetallicMap != 0)
    {
        finalMetallic *= texture(materialMetallicMap, TexCoord).r;
    }
    
    float finalAO = ao;
    if (hasAoMap != 0)
    {
        finalAO *= texture(materialAoMap, TexCoord).r;
    }
    
    vec3 finalEmissive = emissive;
    float finalEmissiveIntensity = emissiveIntensity;
    if (hasEmissiveMap != 0)
    {
        vec4 emissiveSample = texture(materialEmissiveMap, TexCoord);
        // Blend approach: use texture RGB, and combine intensities additively or use max
        finalEmissive = emissiveSample.rgb + (emissive * emissiveIntensity);
        finalEmissiveIntensity = max(emissiveSample.a, emissiveIntensity);
    }
   
    
    // Determine shading model (use material setting, can be overridden by global uniform)
    //bool isBlinnPhong = shadingModel == 1;
    bool isBlinnPhong = false;

    // Pack data into G-Buffer
    // RT0: RGB32_UINT (96 bits)
    gBuffer0.r = packAlbedoShadingModel(finalAlbedo, isBlinnPhong);
    gBuffer0.g = packNormal(finalNormal);
    gBuffer0.b = packEmissive(finalEmissive, finalEmissiveIntensity);
    
    // RT1: R32_UINT (32 bits)
    gBuffer1.r = packMaterialProperties(finalMetallic, finalRoughness, finalAO, normalStrength);
}