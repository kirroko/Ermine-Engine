#version 460
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec2 vertex_texCoord;

layout(location = 3) in mat4 instance_model;
layout(location = 7) in mat3 instance_normal;
//layout(location = 10) in vec4 instance_colour;

out vec2 TexCoord;
out vec3 LightIntensity;
//out vec4 TintColour; // pass to fragment

uniform mat4 view;
uniform mat4 projection;

uniform struct LightInfo {
    vec4 Position;
    vec3 La;
    vec3 Ld;
    vec3 Ls;
} Light;

uniform struct MaterialInfo {
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float Shininess;
} Material;

vec3 phongModel(vec3 position, vec3 n) {
    vec3 ambient = Light.La * Material.Ka;
    vec3 s = normalize(vec3(Light.Position - vec4(position, 1.0)));
    float sDotN = max(dot(s, n), 0.0);
    vec3 diffuse = Light.Ld * Material.Kd * sDotN;
    vec3 spec = vec3(0.0);
    if (sDotN > 0.0) {
        vec3 v = normalize(-position);
        vec3 r = reflect(-s, n);
        float rDotV = max(dot(r, v), 0.0);
        spec = Light.Ls * Material.Ks * pow(rDotV, Material.Shininess);
    }
    return ambient + diffuse + spec;
}

void main() {
    vec3 norm = normalize(instance_normal * vertex_normal);
    vec4 worldPos = instance_model * vec4(vertex_position, 1.0);

    // camera space position
    vec4 camCoords = view * worldPos;
    LightIntensity = phongModel(camCoords.xyz, norm);

    TexCoord = vertex_texCoord;
    //TintColour = instance_colour;

    gl_Position = projection * view * worldPos;
}