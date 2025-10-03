#version 460

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;   // Add tangent attribute

// Skinning attributes
layout (location = 4) in ivec4 aBoneIDs;
layout (location = 5) in vec4 aWeights;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec3 ViewPos; // Position in view space for lighting calculations
out vec3 Tangent;
out vec3 Bitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 NormalMatrix; // Already calculated in Renderer.cpp

uniform bool u_UseSkinning;
uniform mat4 u_BoneMatrices[128]; // bone transforms from Animator

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
    vec4 worldPos = model * skinnedPos;
    vec4 viewPos = view * worldPos;

    gl_Position = projection * viewPos;
    TexCoord = aTexCoord;

    // Calculate fragment position in world space for potential future use
    FragPos = worldPos.xyz;

    // Calculate view space position
    ViewPos = viewPos.xyz;

    // Transform normal to view space
    Normal = normalize(NormalMatrix * skinnedNormal);
    
    // Calculate tangent and bitangent for normal mapping
    Tangent = normalize(NormalMatrix * skinnedTangent);
    
    // Calculate bitangent using cross product (assuming right-handed coordinate system)
    // Note: Some models may have pre-calculated bitangents or handedness info
    Bitangent = normalize(cross(Normal, Tangent));
    
    // Gram-Schmidt process to re-orthogonalize TBN vectors
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    Bitangent = cross(Normal, Tangent);
}