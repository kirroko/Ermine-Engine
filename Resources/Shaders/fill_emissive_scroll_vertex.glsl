#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in ivec4 aBoneIDs;
layout(location = 5) in vec4 aBoneWeights;

struct DrawInfo {
    mat4 modelMatrix;
    vec3 aabbMin;
    uint materialIndex;
    vec3 aabbMax;
    uint entityID;
    uint flags;
    uint boneTransformOffset;
    uint _pad0;
    uint _pad1;
    vec4 normalMatrixCol0;
    vec4 normalMatrixCol1;
    vec4 normalMatrixCol2;
};

struct MaterialData {
    vec4 albedo;
    float metallic;
    float roughness;
    float ao;
    float normalStrength;

    vec3 emissive;
    float emissiveIntensity;

    int shadingModel;
    uint textureFlags;
    int castsShadows;
    float fillAmount;

    vec2 uvScale;
    vec2 uvOffset;

    int albedoMapIndex;
    int normalMapIndex;
    int roughnessMapIndex;
    int metallicMapIndex;

    int aoMapIndex;
    int emissiveMapIndex;
    float fillDirOctX;
    float fillDirOctY;
};

layout(std430, binding = 1) restrict readonly buffer DrawInfoBuffer {
    DrawInfo drawInfos[];
};

layout(std430, binding = 2) restrict readonly buffer BoneTransformBuffer {
    mat4 boneTransforms[];
};

layout(std430, binding = 3) restrict readonly buffer MaterialBlock {
    MaterialData materials[];
};

uniform mat4 view;
uniform mat4 projection;
uniform uint baseDrawID;

out vec2 vBaseUV;
flat out vec4 vAlbedo;
flat out vec3 vEmissive;
flat out float vEmissiveIntensity;
flat out uint vTextureFlags;
flat out int vAlbedoMapIndex;
flat out float vFillAmount;
out float vFillCoord;
out vec2 vFillScrollDirUV;

vec2 signNotZero(vec2 v)
{
    return vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

vec3 octDecode(vec2 e)
{
    vec3 v = vec3(e.x, e.y, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0.0) {
        v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    }
    return normalize(v);
}

void main()
{
    DrawInfo drawInfo = drawInfos[baseDrawID + gl_DrawID];
    MaterialData material = materials[drawInfo.materialIndex];

    const uint FLAG_SKINNING = 1u << 0u;
    bool useSkinning = (drawInfo.flags & FLAG_SKINNING) != 0u;

    vec4 skinnedPos = vec4(aPos, 1.0);
    vec3 skinnedNormal = aNormal;
    vec3 skinnedTangent = aTangent;

    if (useSkinning) {
        uint boneOffset = drawInfo.boneTransformOffset;
        vec4 skinnedPosition = vec4(0.0);
        vec3 skinnedNormalVec = vec3(0.0);
        vec3 skinnedTangentVec = vec3(0.0);

        if (aBoneWeights[0] > 0.0) {
            mat4 b = boneTransforms[boneOffset + aBoneIDs[0]];
            skinnedPosition += b * vec4(aPos, 1.0) * aBoneWeights[0];
            skinnedNormalVec += mat3(b) * aNormal * aBoneWeights[0];
            skinnedTangentVec += mat3(b) * aTangent * aBoneWeights[0];
        }
        if (aBoneWeights[1] > 0.0) {
            mat4 b = boneTransforms[boneOffset + aBoneIDs[1]];
            skinnedPosition += b * vec4(aPos, 1.0) * aBoneWeights[1];
            skinnedNormalVec += mat3(b) * aNormal * aBoneWeights[1];
            skinnedTangentVec += mat3(b) * aTangent * aBoneWeights[1];
        }
        if (aBoneWeights[2] > 0.0) {
            mat4 b = boneTransforms[boneOffset + aBoneIDs[2]];
            skinnedPosition += b * vec4(aPos, 1.0) * aBoneWeights[2];
            skinnedNormalVec += mat3(b) * aNormal * aBoneWeights[2];
            skinnedTangentVec += mat3(b) * aTangent * aBoneWeights[2];
        }
        if (aBoneWeights[3] > 0.0) {
            mat4 b = boneTransforms[boneOffset + aBoneIDs[3]];
            skinnedPosition += b * vec4(aPos, 1.0) * aBoneWeights[3];
            skinnedNormalVec += mat3(b) * aNormal * aBoneWeights[3];
            skinnedTangentVec += mat3(b) * aTangent * aBoneWeights[3];
        }

        skinnedPos = skinnedPosition;
        skinnedNormal = skinnedNormalVec;
        skinnedTangent = skinnedTangentVec;
    }

    vec4 worldPos = drawInfo.modelMatrix * skinnedPos;
    gl_Position = projection * view * worldPos;

    vAlbedo = material.albedo;
    vEmissive = material.emissive;
    vEmissiveIntensity = material.emissiveIntensity;
    vTextureFlags = material.textureFlags;
    vAlbedoMapIndex = material.albedoMapIndex;
    vFillAmount = clamp(material.fillAmount, 0.0, 1.0);
    vBaseUV = fma(aTexCoord, material.uvScale, material.uvOffset);

    vec3 fillDir = octDecode(vec2(material.fillDirOctX, material.fillDirOctY));

    const float EPS = 1e-6;
    float p = dot(skinnedPos.xyz, fillDir);
    float minP = dot(drawInfo.aabbMin, fillDir);
    float maxP = dot(drawInfo.aabbMax, fillDir);
    float range = maxP - minP;
    if (range <= EPS) {
        vFillCoord = 0.0;
    } else {
        vFillCoord = clamp((p - minP) / range, 0.0, 1.0);
    }

    // Match gbuffer TBN convention: orthonormalize tangent against normal, then derive bitangent.
    vec3 n = normalize(skinnedNormal);
    vec3 t = normalize(skinnedTangent);
    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);

    // Project local fill direction into tangent-space UV axes.
    vec2 scrollDir = vec2(dot(fillDir, t), dot(fillDir, b));
    float scrollLen = length(scrollDir);
    vFillScrollDirUV = (scrollLen > EPS) ? (scrollDir / scrollLen) : vec2(0.0, 1.0);
}
