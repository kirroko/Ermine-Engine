#version 460

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec2 vertex_texCoord;

out vec2 TexCoord;
out vec3 FragPos;     // Fragment position in view space
out vec3 Normal;      // Normal in view space

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 NormalMatrix;

void main() {
    // Transform position to view space
    vec4 viewPos = view * model * vec4(vertex_position, 1.0);
    FragPos = viewPos.xyz;
    
    // Transform normal to view space
    Normal = normalize(NormalMatrix * vertex_normal);
    
    // Pass texture coordinates
    TexCoord = vertex_texCoord;
    
    // Final position
    gl_Position = projection * viewPos;
}