#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in ivec4 aBoneIDs;      // Only present in SkinnedVAO
layout(location = 5) in vec4 aBoneWeights;    // Only present in SkinnedVAO

// Draw info structure matching CPU-side DrawInfo (std430 layout)
// Total size: 160 bytes (must match C++ DrawInfo in DrawCommands.h)
struct DrawInfo {
    mat4 modelMatrix;           // 64 bytes (offset 0-63) - Model transformation matrix
    vec3 aabbMin;               // 12 bytes (offset 64-75) - AABB minimum bounds
    uint materialIndex;         // 4 bytes (offset 76-79) - Index into material SSBO
    vec3 aabbMax;               // 12 bytes (offset 80-91) - AABB maximum bounds
    uint entityID;              // 4 bytes (offset 92-95) - Entity ID for identification
    uint flags;                 // 4 bytes (offset 96-99) - Flags (bit 0: useSkinning)
    uint boneTransformOffset;   // 4 bytes (offset 100-103) - Starting index in skeletal SSBO
    uint _pad0;                 // 4 bytes (offset 104-107) - Padding
    uint _pad1;                 // 4 bytes (offset 108-111) - Padding to 16-byte boundary
    vec4 normalMatrixCol0;      // 16 bytes (offset 112-127) - Normal matrix column 0 (xyz used)
    vec4 normalMatrixCol1;      // 16 bytes (offset 128-143) - Normal matrix column 1 (xyz used)
    vec4 normalMatrixCol2;      // 16 bytes (offset 144-159) - Normal matrix column 2 (xyz used)
};

// SSBO bindings (still used for DrawInfo and bone transforms)
layout(std430, binding = 1) restrict readonly buffer DrawInfoBuffer {
    DrawInfo drawInfos[];
};

// Skeletal animation bone transforms SSBO (Binding 2)
layout(std430, binding = 2) restrict readonly buffer BoneTransformBuffer {
    mat4 boneTransforms[]; // All bone transforms for all entities
};

// Transformation matrices
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalView; // Normal matrix for view (mat3 of view matrix) - calculated once on CPU

// Base draw ID offset for multi-batch rendering
uniform uint baseDrawID;

// Outputs to fragment shader
out vec2 TexCoord;
out vec3 WorldPos;
out vec3 WorldNormal;
out vec3 ViewPos;
out vec3 ViewNormal;
out vec3 ViewTangent;
out vec3 ViewBitangent;
flat out uint vMaterialIndex; // Pass material index to fragment shader

void main()
{
    // Get draw info for this draw call (offset by baseDrawID for multi-batch rendering)
    DrawInfo drawInfo = drawInfos[baseDrawID + gl_DrawID];
    mat4 model = drawInfo.modelMatrix;

    // Reconstruct normal matrix from 3 separate columns (pre-calculated on CPU, eliminates expensive inverse() per vertex)
    mat3 normalMatrix = mat3(drawInfo.normalMatrixCol0.xyz, drawInfo.normalMatrixCol1.xyz, drawInfo.normalMatrixCol2.xyz);

    // Extract useSkinning flag from bit 0 of flags
    bool useSkinning = (drawInfo.flags & 1u) != 0u;


    vec4 skinnedPos = vec4(aPos, 1.0);
    vec3 skinnedNormal  = aNormal;
    vec3 skinnedTangent = aTangent;

    // Apply skeletal animation if enabled
    if (useSkinning) {
        // Get bone offset for this entity from DrawInfo
        uint boneOffset = drawInfo.boneTransformOffset;

        // Optimized conditional bone skinning - skip zero-weight bones
        // Processes positions, normals, and tangents
        vec4 skinnedPosition = vec4(0.0);
        vec3 skinnedNormalVec = vec3(0.0);
        vec3 skinnedTangentVec = vec3(0.0);

        // Bone 0 - skip if zero weight
        if (aBoneWeights[0] > 0.0) {
            mat4 boneTransform = boneTransforms[boneOffset + aBoneIDs[0]];
            skinnedPosition += boneTransform * vec4(aPos, 1.0) * aBoneWeights[0];
            skinnedNormalVec += mat3(boneTransform) * aNormal * aBoneWeights[0];
            skinnedTangentVec += mat3(boneTransform) * aTangent * aBoneWeights[0];
        }

        // Bone 1
        if (aBoneWeights[1] > 0.0) {
            mat4 boneTransform = boneTransforms[boneOffset + aBoneIDs[1]];
            skinnedPosition += boneTransform * vec4(aPos, 1.0) * aBoneWeights[1];
            skinnedNormalVec += mat3(boneTransform) * aNormal * aBoneWeights[1];
            skinnedTangentVec += mat3(boneTransform) * aTangent * aBoneWeights[1];
        }

        // Bone 2
        if (aBoneWeights[2] > 0.0) {
            mat4 boneTransform = boneTransforms[boneOffset + aBoneIDs[2]];
            skinnedPosition += boneTransform * vec4(aPos, 1.0) * aBoneWeights[2];
            skinnedNormalVec += mat3(boneTransform) * aNormal * aBoneWeights[2];
            skinnedTangentVec += mat3(boneTransform) * aTangent * aBoneWeights[2];
        }

        // Bone 3
        if (aBoneWeights[3] > 0.0) {
            mat4 boneTransform = boneTransforms[boneOffset + aBoneIDs[3]];
            skinnedPosition += boneTransform * vec4(aPos, 1.0) * aBoneWeights[3];
            skinnedNormalVec += mat3(boneTransform) * aNormal * aBoneWeights[3];
            skinnedTangentVec += mat3(boneTransform) * aTangent * aBoneWeights[3];
        }

        skinnedPos = skinnedPosition;
        skinnedNormal = skinnedNormalVec;
        skinnedTangent = skinnedTangentVec;
    }

    // World space position (for lighting calculations)
    vec4 worldPos = model * skinnedPos;
    WorldPos = worldPos.xyz;

    // View space position
    vec4 viewPos = view * worldPos;
    ViewPos = viewPos.xyz;

    // Final vertex position
    gl_Position = projection * viewPos;

    // Pass through texture coordinates
    TexCoord = aTexCoord;

    // Transform normal to world space using pre-calculated normal matrix (avoids expensive inverse())
    WorldNormal = normalize(normalMatrix * skinnedNormal);

    // Calculate combined model-view normal matrix for view-space TBN
    // Still cheaper than calculating inverse() per vertex
    mat3 modelViewNormalMatrix = normalView * normalMatrix;

    // Transform TBN directly to view space
    // This eliminates redundant intermediate normalizations
    ViewNormal = normalize(modelViewNormalMatrix * skinnedNormal);
    ViewTangent = normalize(modelViewNormalMatrix * skinnedTangent);

    // Calculate bitangent in view space
    ViewBitangent = cross(ViewNormal, ViewTangent);

    // Re-orthogonalize TBN vectors in view space using Gram-Schmidt process
    // Only tangent needs correction; bitangent is recalculated from corrected tangent
    ViewTangent = normalize(ViewTangent - dot(ViewTangent, ViewNormal) * ViewNormal);
    ViewBitangent = cross(ViewNormal, ViewTangent);

    // Pass material index to fragment shader
    vMaterialIndex = drawInfo.materialIndex;
}