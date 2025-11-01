#version 460 core

// Vertex attributes from VBO (via VAO)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in ivec4 aBoneIDs;      // Only present in SkinnedVAO
layout(location = 5) in vec4 aBoneWeights;    // Only present in SkinnedVAO

// Draw info structure matching CPU-side DrawInfo (std430 layout)
struct DrawInfo {
    mat4 modelMatrix;           // 64 bytes - Model transformation matrix
    vec3 aabbMin;               // 12 bytes - AABB minimum bounds
    uint materialIndex;         // 4 bytes - Index into material SSBO
    vec3 aabbMax;               // 12 bytes - AABB maximum bounds
    uint entityID;              // 4 bytes - Entity ID
    uint flags;                 // 4 bytes - Flags (bit 0: useSkinning)
    uint boneTransformOffset;   // 4 bytes - Starting index in skeletal SSBO
    uint _pad[2];               // 8 bytes - Padding
};

// SSBO bindings
layout(std430, binding = 3) restrict readonly buffer DrawInfoBuffer {
    DrawInfo drawInfos[];
};

// Skeletal animation bone transforms SSBO (Binding 7)
layout(std430, binding = 7) restrict readonly buffer BoneTransformBuffer {
    mat4 boneTransforms[];
};

// Transformation matrices
uniform mat4 view;
uniform mat4 projection;

// Base draw ID offset for multi-batch rendering
uniform uint baseDrawID;

// Outputs to fragment shader
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec3 ViewPos;
out vec3 Tangent;
out vec3 Bitangent;
flat out uint vMaterialIndex;

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
    }

    // Calculate normal matrix from model matrix
    mat3 NormalMatrix = transpose(inverse(mat3(model)));

    // World & View
    vec4 worldPos = model * skinnedPos;
    vec4 viewPos = view * worldPos;

    gl_Position = projection * viewPos;
    TexCoord = aTexCoord;

    // Calculate fragment position in world space
    FragPos = worldPos.xyz;

    // Calculate view space position
    ViewPos = viewPos.xyz;

    // Transform normal to view space
    Normal = normalize(NormalMatrix * skinnedNormal);

    // Calculate tangent and bitangent for normal mapping
    Tangent = normalize(NormalMatrix * skinnedTangent);

    // Calculate bitangent using cross product
    Bitangent = normalize(cross(Normal, Tangent));

    // Gram-Schmidt process to re-orthogonalize TBN vectors
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    Bitangent = cross(Normal, Tangent);

    // Pass material index to fragment shader
    vMaterialIndex = drawInfo.materialIndex;
}