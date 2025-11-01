#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in ivec4 aBoneIDs;      // Only present in SkinnedVAO
layout(location = 5) in vec4 aBoneWeights;    // Only present in SkinnedVAO

// Draw info structure matching CPU-side DrawInfo (std430 layout)
// Total size: 112 bytes (must match C++ DrawInfo in DrawCommands.h)
struct DrawInfo {
    mat4 modelMatrix;           // 64 bytes (offset 0-63) - Model transformation matrix
    vec3 aabbMin;               // 12 bytes (offset 64-75) - AABB minimum bounds
    uint materialIndex;         // 4 bytes (offset 76-79) - Index into material SSBO
    vec3 aabbMax;               // 12 bytes (offset 80-91) - AABB maximum bounds
    uint entityID;              // 4 bytes (offset 92-95) - Entity ID for identification
    uint flags;                 // 4 bytes (offset 96-99) - Flags (bit 0: useSkinning)
    uint boneTransformOffset;   // 4 bytes (offset 100-103) - Starting index in skeletal SSBO
    uint _pad[2];               // 8 bytes (offset 104-111) - Explicit padding to 16-byte alignment
};

// SSBO bindings (still used for DrawInfo and bone transforms)
layout(std430, binding = 1) restrict readonly buffer DrawInfoBuffer {
    DrawInfo drawInfos[];
};

// Skeletal animation bone transforms SSBO (Binding 7)
layout(std430, binding = 2) restrict readonly buffer BoneTransformBuffer {
    mat4 boneTransforms[]; // All bone transforms for all entities
};

// Pre-skinned positions output (Binding 8) - for shadow pass reuse
layout(std430, binding = 4) restrict writeonly buffer PreSkinnedBuffer {
    vec4 preSkinnedPositions[];  // xyz = skinned position, w = unused
};

// Transformation matrices
uniform mat4 view;
uniform mat4 projection;

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

    // Extract useSkinning flag from bit 0 of flags
    bool useSkinning = (drawInfo.flags & 1u) != 0u;


    vec4 skinnedPos = vec4(aPos, 1.0);
    vec3 skinnedNormal  = aNormal;
    vec3 skinnedTangent = aTangent;

    // Apply skeletal animation if enabled
    if (useSkinning) {
        // Get bone offset for this entity from DrawInfo
        uint boneOffset = drawInfo.boneTransformOffset;

        // Calculate final bone transform using weighted blend
        mat4 boneTransform =
            boneTransforms[boneOffset + aBoneIDs[0]] * aBoneWeights[0] +
            boneTransforms[boneOffset + aBoneIDs[1]] * aBoneWeights[1] +
            boneTransforms[boneOffset + aBoneIDs[2]] * aBoneWeights[2] +
            boneTransforms[boneOffset + aBoneIDs[3]] * aBoneWeights[3];

        skinnedPos     = boneTransform * vec4(aPos, 1.0);
        skinnedNormal  = mat3(boneTransform) * aNormal;
        skinnedTangent = mat3(boneTransform) * aTangent;

        // Write skinned position to SSBO for shadow pass reuse (eliminates redundant calculations)
        preSkinnedPositions[gl_VertexID] = skinnedPos;
    }

    // Calculate normal matrix from model matrix
    mat3 NormalMatrix = transpose(inverse(mat3(model)));

    // World & View
    // Calculate world space position
    vec4 worldPos = model * skinnedPos;
    WorldPos = worldPos.xyz;    
    
    // Calculate view space position
    vec4 viewPos = view * worldPos;
    ViewPos = viewPos.xyz;
    
    // Final vertex position
    gl_Position = projection * viewPos;
    
    // Pass through texture coordinates
    TexCoord = aTexCoord;

    // Transform normal to world space using normal matrix
    WorldNormal = normalize(NormalMatrix * skinnedNormal);
    
    // Transform normal to view space
    ViewNormal = normalize(mat3(view) * WorldNormal);
    
    // Transform tangent to world space first, then to view space
    vec3 worldTangent = normalize(NormalMatrix * skinnedTangent);
    ViewTangent = normalize(mat3(view) * worldTangent);
    
    // Calculate bitangent in world space first, then transform to view space
    vec3 worldBitangent = normalize(cross(WorldNormal, worldTangent));
    ViewBitangent = normalize(mat3(view) * worldBitangent);
    
    // Re-orthogonalize TBN vectors in view space using Gram-Schmidt process
    ViewTangent = normalize(ViewTangent - dot(ViewTangent, ViewNormal) * ViewNormal);
    ViewBitangent = normalize(cross(ViewNormal, ViewTangent));

    // Pass material index to fragment shader
    vMaterialIndex = drawInfo.materialIndex;
}