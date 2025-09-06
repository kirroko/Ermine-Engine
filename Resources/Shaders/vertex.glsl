#version 460
#define MAX_LIGHTS 16

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec2 vertex_texCoord;

out vec2 TexCoord;
out vec3 LightIntensity;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool useBF;

uniform mat3 NormalMatrix;

struct LightData {
    vec4 position_type;    // xyz = pos (view), w = type
    vec4 color_intensity;  // rgb = color, a = intensity
    vec4 direction_range;  // xyz = dir (view), w = range
    vec4 spot_angles;      // x = innerCos, y = outerCos
};

layout(std140, binding = 1) uniform Lights {
    vec4 uLightCount; // x = light count
    LightData uLights[MAX_LIGHTS];
};

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
/*
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
*/

vec3 blinnPhongForLight(const in LightData ld, const in vec3 fragPos, const in vec3 normal) {
    vec3 lightColor = ld.color_intensity.rgb * ld.color_intensity.a;
    int type = int(ld.position_type.w + 0.5); // Safe cast to int

    vec3 L;           // Light direction (toward fragment)
    float attenuation = 1.0;

    if (type == 0) {
        // Directional light: direction is stored in direction_range.xyz (from light)
        L = normalize(ld.direction_range.xyz);
    } else {
        // Point or Spot: light position in view space
        vec3 lightPos = ld.position_type.xyz;
        vec3 fragToLight = lightPos - fragPos;
        float dist = length(fragToLight);
        L = fragToLight / dist; // normalize

        // Radial attenuation (linear falloff)
        float range = ld.direction_range.w;
        if (range > 0.0) {
            attenuation = max(1.0 - (dist / range), 0.0);
        }

        // Spotlight modulation
        if (type == 2) {
            vec3 spotDir = normalize(ld.direction_range.xyz); // spotlight direction (where it points)
            float spotCos = dot(-spotDir, L); // angle between light beam and direction to fragment

            float innerCos = ld.spot_angles.x;
            float outerCos = ld.spot_angles.y;

            // Smooth falloff from outer to inner
            float spotFactor = smoothstep(outerCos, innerCos, spotCos);
            attenuation *= spotFactor;
        }
    }

    // Diffuse
    float NdotL = max(dot(normal, L), 0.0);
    vec3 diffuse = Material.Kd * lightColor * NdotL * attenuation;

    // Ambient (not view-dependent)
    vec3 ambient = Material.Ka * lightColor * attenuation; // optional: ambient doesn't usually attenuate

    // Specular (Blinn-Phong)
    vec3 specular = vec3(0.0);
    if (NdotL > 0.0) {
        vec3 V = normalize(-fragPos); // View direction in view space
        vec3 H = normalize(L + V);    // Halfway vector
        float specFactor = pow(max(dot(normal, H), 0.0), Material.Shininess);
        specular = Material.Ks * lightColor * specFactor * attenuation;
    }

    return ambient + diffuse + specular;
}

// Evaluate all lights
vec3 computeLighting(const in vec3 fragPos, const in vec3 normal) {
    vec3 totalLight = vec3(0.0);

    int count = int(uLightCount.x);
    count = min(count, MAX_LIGHTS); // Safety

    for (int i = 0; i < count; ++i) {
        totalLight += blinnPhongForLight(uLights[i], fragPos, normal);
    }

    return totalLight;
}

void main() {
    // Get the position and normal in camera space
    vec3 camNorm, camPosition;
    getCamSpace(camNorm, camPosition);
    
    // Evaluate the reflection model
    LightIntensity = computeLighting(camPosition, camNorm);
    
    TexCoord = vertex_texCoord;
    
    gl_Position = projection * view * model * vec4(vertex_position, 1.0);
}