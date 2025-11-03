#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

// Skinning attributes
layout (location = 4) in ivec4 aBoneIDs;
layout (location = 5) in vec4 aWeights;

// Transformation matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 NormalMatrix;

uniform bool u_UseSkinning;
uniform mat4 u_BoneMatrices[128]; // bone transforms from Animator

// Outputs to fragment shader
out vec2 TexCoord;
out vec3 WorldPos;
out vec3 WorldNormal;
out vec3 ViewPos;
out vec3 ViewNormal;
out vec3 ViewTangent;
out vec3 ViewBitangent;

void main()
{
    vec4 skinnedPos = vec4(aPos, 1.0);
    vec3 skinnedNormal  = aNormal;
    vec3 skinnedTangent = aTangent;

    // Does it use skinning
    if (u_UseSkinning) {
        mat4 boneTransform =
            u_BoneMatrices[aBoneIDs[0]] * aWeights[0] +
            u_BoneMatrices[aBoneIDs[1]] * aWeights[1] +
            u_BoneMatrices[aBoneIDs[2]] * aWeights[2] +
            u_BoneMatrices[aBoneIDs[3]] * aWeights[3];
        
        skinnedPos     = boneTransform * vec4(aPos, 1.0);
        skinnedNormal  = mat3(boneTransform) * aNormal;
        skinnedTangent = mat3(boneTransform) * aTangent;
    }

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
}