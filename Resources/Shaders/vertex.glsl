#version 460

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;   // Add tangent attribute

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

void main()
{
    // Calculate positions
    vec4 viewPos = view * model * vec4(aPos, 1.0);
    ViewPos = viewPos.xyz;
    
    gl_Position = projection * viewPos;
    TexCoord = aTexCoord;

    // Transform normal to view space
    Normal = normalize(NormalMatrix * aNormal);

    // Calculate fragment position in world space for potential future use
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Calculate tangent and bitangent for normal mapping
    Tangent = normalize(NormalMatrix * aTangent);
    
    // Calculate bitangent using cross product (assuming right-handed coordinate system)
    // Note: Some models may have pre-calculated bitangents or handedness info
    Bitangent = normalize(cross(Normal, Tangent));
    
    // Gram-Schmidt process to re-orthogonalize TBN vectors
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    Bitangent = cross(Normal, Tangent);
}