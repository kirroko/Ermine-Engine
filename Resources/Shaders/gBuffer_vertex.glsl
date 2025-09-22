#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

// Transformation matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 NormalMatrix;

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
    // Calculate world space position
    vec4 worldPos = model * vec4(aPos, 1.0);
    WorldPos = worldPos.xyz;    
    
    // Calculate view space position
    vec4 viewPos = view * worldPos;
    ViewPos = viewPos.xyz;
    
    // Final vertex position
    gl_Position = projection * viewPos;
    
    // Pass through texture coordinates
    TexCoord = aTexCoord;

    // Transform normal to world space using normal matrix
    WorldNormal = normalize(NormalMatrix * aNormal);
    
    // Transform normal to view space
    ViewNormal = normalize(mat3(view) * WorldNormal);
    
    // Transform tangent to world space first, then to view space
    vec3 worldTangent = normalize(NormalMatrix * aTangent);
    ViewTangent = normalize(mat3(view) * worldTangent);
    
    // Calculate bitangent in world space first, then transform to view space
    vec3 worldBitangent = normalize(cross(WorldNormal, worldTangent));
    ViewBitangent = normalize(mat3(view) * worldBitangent);
    
    // Re-orthogonalize TBN vectors in view space using Gram-Schmidt process
    ViewTangent = normalize(ViewTangent - dot(ViewTangent, ViewNormal) * ViewNormal);
    ViewBitangent = normalize(cross(ViewNormal, ViewTangent));
}