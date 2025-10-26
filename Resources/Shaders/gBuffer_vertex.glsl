#version 460 core

// Vertex structure matching CPU-side Vertex (std430 layout)
// Total size: 64 bytes (must match C++ Vertex in MeshTypes.h)
struct Vertex {
    vec3 position;      // offset 0, size 12 bytes
    float _pad0;        // offset 12, size 4 bytes (padding to reach 16)
    vec3 normal;        // offset 16, size 12 bytes
    float _pad1;        // offset 28, size 4 bytes (padding to reach 32)
    vec2 texCoord;      // offset 32, size 8 bytes
    float _pad2[2];     // offset 40, size 8 bytes (padding to reach 48)
    vec3 tangent;       // offset 48, size 12 bytes
    float _pad3;        // offset 60, size 4 bytes (padding to reach 64)
};

// Skinned vertex structure matching CPU-side SkinnedVertex (std430 layout)
// Total size: 96 bytes (must match C++ SkinnedVertex in MeshTypes.h)
struct SkinnedVertex {
    vec3 position;      // offset 0, size 12 bytes
    float _pad0;        // offset 12, size 4 bytes (padding to reach 16)
    vec3 normal;        // offset 16, size 12 bytes
    float _pad1;        // offset 28, size 4 bytes (padding to reach 32)
    vec2 texCoord;      // offset 32, size 8 bytes
    float _pad2[2];     // offset 40, size 8 bytes (padding to reach 48)
    vec3 tangent;       // offset 48, size 12 bytes
    float _pad3;        // offset 60, size 4 bytes (padding to reach 64)
    ivec4 boneIDs;      // offset 64, size 16 bytes
    vec4 boneWeights;   // offset 80, size 16 bytes
};

// Draw info structure matching CPU-side DrawInfo (std430 layout)
// Total size: 112 bytes (must match C++ DrawInfo in DrawCommands.h)
struct DrawInfo {
    mat4 modelMatrix;       // 64 bytes (offset 0-63) - Model transformation matrix
    vec3 aabbMin;           // 12 bytes (offset 64-75) - AABB minimum bounds
    uint materialIndex;     // 4 bytes (offset 76-79) - Index into material SSBO
    vec3 aabbMax;           // 12 bytes (offset 80-91) - AABB maximum bounds
    uint entityID;          // 4 bytes (offset 92-95) - Entity ID for identification
    uint flags;             // 4 bytes (offset 96-99) - Flags (bit 0: useSkinning)
    uint _pad[3];           // 12 bytes (offset 100-111) - Explicit padding to 16-byte alignment
};

// SSBO bindings
layout(std430, binding = 0) restrict readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(std430, binding = 1) restrict readonly buffer IndexBuffer {
    uint indices[];
};

layout(std430, binding = 3) restrict readonly buffer DrawInfoBuffer {
    DrawInfo drawInfos[];
};

layout(std430, binding = 4) restrict readonly buffer SkinnedVertexBuffer {
    SkinnedVertex skinnedVertices[];
};

// Transformation matrices
uniform mat4 view;
uniform mat4 projection;

uniform mat4 u_BoneMatrices[128]; // bone transforms from Animator

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
    // Get draw info for this draw call
    DrawInfo drawInfo = drawInfos[gl_DrawID];
    mat4 model = drawInfo.modelMatrix;

    // Extract useSkinning flag from bit 0 of flags
    bool useSkinning = (drawInfo.flags & 1u) != 0u;

    vec3 aPos;
    vec3 aNormal;
    vec2 aTexCoord;
    vec3 aTangent;
    ivec4 aBoneIDs = ivec4(0);
    vec4 aWeights = vec4(0.0);

    // Fetch vertex data from appropriate buffer
    if (useSkinning) {
        SkinnedVertex sv = skinnedVertices[gl_VertexID];
        aPos = sv.position;
        aNormal = sv.normal;
        aTexCoord = sv.texCoord;
        aTangent = sv.tangent;
        aBoneIDs = sv.boneIDs;
        aWeights = sv.boneWeights;
    } else {
        Vertex v = vertices[gl_VertexID];
        aPos = v.position;
        aNormal = v.normal;
        aTexCoord = v.texCoord;
        aTangent = v.tangent;
    }

    vec4 skinnedPos = vec4(aPos, 1.0);
    vec3 skinnedNormal  = aNormal;
    vec3 skinnedTangent = aTangent;

    // Does it use skinning
    if (useSkinning) {
        mat4 boneTransform =
            u_BoneMatrices[aBoneIDs[0]] * aWeights[0] +
            u_BoneMatrices[aBoneIDs[1]] * aWeights[1] +
            u_BoneMatrices[aBoneIDs[2]] * aWeights[2] +
            u_BoneMatrices[aBoneIDs[3]] * aWeights[3];
        
        skinnedPos     = boneTransform * vec4(aPos, 1.0);
        skinnedNormal  = mat3(boneTransform) * aNormal;
        skinnedTangent = mat3(boneTransform) * aTangent;
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