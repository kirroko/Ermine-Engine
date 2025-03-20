#version 460
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec2 vertex_texCoord;

out vec2 TexCoord;
out vec3 LightIntensity;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat3 NormalMatrix;

uniform struct LightInfo {
    vec4 Position; // Light position in eye coords
    vec3 La;      // Ambient light intensity
    vec3 Ld;      // Diffuse light intensity
    vec3 Ls;      // Specular light intensity
} Light;

uniform struct MaterialInfo {
    vec3 Ka;      // Ambient reflectivity
    vec3 Kd;      // Diffuse reflectivity
    vec3 Ks;      // Specular reflectivity
    float Shininess; // Specular shininess factor
} Material;

void getCamSpace(out vec3 norm, out vec3 position)
{
    norm = normalize(NormalMatrix * vertex_normal);
    vec4 camCoords = view * model * vec4(vertex_position, 1.0);
    position = camCoords.xyz;
}

vec3 phongModel(vec3 position, vec3 n)
{
    vec3 ambient = Light.La * Material.Ka;
    vec3 s = normalize(vec3(Light.Position - vec4(position, 1.0)));
    float sDotN = max(dot(s, n), 0.0);
    vec3 diffuse = Light.Ld * Material.Kd * sDotN;
    vec3 spec = vec3(0.0);
    if(sDotN > 0.0)
    {
        vec3 v = normalize(-position);
        vec3 r = reflect(-s, n);
        float rDotV = max(dot(r, v), 0.0);
        spec = Light.Ls * Material.Ks * pow(rDotV, Material.Shininess);
    }
    return ambient + diffuse + spec;
}

void main() {
    // Get the position and normal in camera space
    vec3 camNorm, camPosition;
    getCamSpace(camNorm, camPosition);
    
    // Evaluate the reflection model
    LightIntensity = phongModel(camPosition, camNorm);
    
    TexCoord = vertex_texCoord;
    
    gl_Position = projection * view * model * vec4(vertex_position, 1.0);
}