#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

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
}