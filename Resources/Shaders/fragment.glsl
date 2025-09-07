#version 460
#define MAX_LIGHTS 16

in vec2 TexCoord;
in vec3 FragPos;      // Fragment position in view space
in vec3 Normal;       // Normal in view space

out vec4 fragColor;

uniform sampler2D ourTexture;

// Shading mode toggle
uniform bool isBlinnPhong; // true = Blinn-Phong, false = PBR

// View position for PBR calculations
uniform vec3 viewPos;

// Blinn-Phong material properties
uniform vec3 materialKa;
uniform vec3 materialKd;
uniform vec3 materialKs;
uniform float materialShininess;

// PBR material properties
uniform vec3 pbrAlbedo;
uniform float pbrMetallic;
uniform float pbrRoughness;
uniform float pbrAO;

struct LightData {
    vec4 position_type;    // xyz = pos (view), w = type (0=POINT, 1=DIRECTIONAL, 2=SPOT)
    vec4 color_intensity;  // rgb = color, a = intensity
    vec4 direction_range;  // xyz = dir (view), w = range
    vec4 spot_angles;      // x = innerCos, y = outerCos
};

layout(std140, binding = 1) uniform Lights {
    vec4 uLightCount; // x = light count
    LightData uLights[MAX_LIGHTS];
};

// Helper functions for PBR
vec3 getNormalFromMap() {
    return normalize(Normal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359 * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Blinn-Phong lighting for a single light
vec3 blinnPhongForLight(const in LightData ld, const in vec3 fragPos, const in vec3 normal) {
    vec3 lightColor = ld.color_intensity.rgb * ld.color_intensity.a;
    int type = int(ld.position_type.w + 0.5); // Safe cast to int

    vec3 L;           // Light direction (toward light)
    float attenuation = 1.0;

    if (type == 1) {
        // Directional light
        L = normalize(-ld.direction_range.xyz); // Direction points toward light
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
            vec3 spotDir = normalize(ld.direction_range.xyz); 
            float spotCos = dot(-spotDir, L);

            float innerCos = ld.spot_angles.x;
            float outerCos = ld.spot_angles.y;

            // Smooth falloff from outer to inner
            float spotFactor = smoothstep(outerCos, innerCos, spotCos);
            attenuation *= spotFactor;
        }
    }

    // Diffuse
    float NdotL = max(dot(normal, L), 0.0);
    vec3 diffuse = materialKd * lightColor * NdotL * attenuation;

    // Ambient (simple approximation)
    vec3 ambient = materialKa * lightColor * 0.1; // Small ambient factor

    // Specular (Blinn-Phong)
    vec3 specular = vec3(0.0);
    if (NdotL > 0.0) {
        vec3 V = normalize(-fragPos); // View direction in view space
        vec3 H = normalize(L + V);    // Halfway vector
        float specFactor = pow(max(dot(normal, H), 0.0), materialShininess);
        specular = materialKs * lightColor * specFactor * attenuation;
    }

    return ambient + diffuse + specular;
}

// PBR lighting for a single light
vec3 pbrForLight(const in LightData ld, const in vec3 fragPos, const in vec3 normal, const in vec3 viewDir, const in vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 lightColor = ld.color_intensity.rgb * ld.color_intensity.a;
    int type = int(ld.position_type.w + 0.5);

    vec3 L;
    float attenuation = 1.0;

    if (type == 1) {
        // Directional light
        L = normalize(-ld.direction_range.xyz);
    } else {
        // Point or Spot
        vec3 lightPos = ld.position_type.xyz;
        vec3 fragToLight = lightPos - fragPos;
        float dist = length(fragToLight);
        L = fragToLight / dist;

        // Attenuation
        float range = ld.direction_range.w;
        if (range > 0.0) {
            attenuation = max(1.0 - (dist / range), 0.0);
        }

        // Spotlight
        if (type == 2) {
            vec3 spotDir = normalize(ld.direction_range.xyz);
            float spotCos = dot(-spotDir, L);
            float innerCos = ld.spot_angles.x;
            float outerCos = ld.spot_angles.y;
            float spotFactor = smoothstep(outerCos, innerCos, spotCos);
            attenuation *= spotFactor;
        }
    }

    vec3 H = normalize(viewDir + L);
    float NdotV = max(dot(normal, viewDir), 0.0);
    float NdotL = max(dot(normal, L), 0.0);
    float HdotV = max(dot(H, viewDir), 0.0);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, viewDir, L, roughness);
    vec3 F = fresnelSchlick(HdotV, F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 radiance = lightColor * attenuation;
    
    return (kD * albedo / 3.14159265359 + specular) * radiance * NdotL;
}

// Compute all lighting
vec3 computeBlinnPhongLighting(const in vec3 fragPos, const in vec3 normal) {
    vec3 totalLight = vec3(0.0);

    int count = int(uLightCount.x);
    count = min(count, MAX_LIGHTS);

    for (int i = 0; i < count; ++i) {
        totalLight += blinnPhongForLight(uLights[i], fragPos, normal);
    }

    return totalLight;
}

vec3 computePBRLighting(const in vec3 fragPos, const in vec3 normal, const in vec3 viewDir, const in vec3 albedo, float metallic, float roughness) {
    // Calculate F0 for dielectrics and metals
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    int count = int(lightCount.x);
    count = min(count, MAX_LIGHTS);

    for (int i = 0; i < count; ++i) {
        Lo += pbrForLight(lights[i], fragPos, normal, viewDir, albedo, metallic, roughness, F0);
    }

    // Simple ambient lighting
    vec3 ambient = vec3(0.03) * albedo * pbrAO;
    vec3 color = ambient + Lo;

    // HDR tonemapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    return color;
}

void main() {
    vec3 normal = getNormalFromMap();
    vec4 texColor = texture(ourTexture, TexCoord);
    
    vec3 result;
    
    if (isBlinnPhong) {
        // Blinn-Phong shading
        vec3 lightIntensity = computeBlinnPhongLighting(FragPos, normal);
        result = texColor.rgb * lightIntensity;
    } else {
        // PBR shading
        vec3 viewDir = normalize(-FragPos); // View direction in view space
        vec3 albedo = texColor.rgb * pbrAlbedo;
        
        result = computePBRLighting(FragPos, normal, viewDir, albedo, pbrMetallic, pbrRoughness);
    }
    
    fragColor = vec4(result, texColor.a);
}