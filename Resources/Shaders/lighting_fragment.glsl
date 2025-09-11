#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

// G-Buffer textures - Use usampler2D for integer textures
uniform usampler2D u_GBuffer0;    // RT0: RGB32_UINT (Albedo + Normal + Emissive)  
uniform usampler2D u_GBuffer1;    // RT1: RG32_UINT (Material + Motion vectors)
uniform sampler2D u_GBufferDepth; // Depth buffer

// Matrices for position reconstruction
uniform mat4 view;
uniform mat4 invView;      
uniform mat4 invProjection;   

// Shading mode
uniform int u_ShadingMode; // 0 = PBR, 1 = Blinn-Phong

// Light structure - must match C++ LightGPU
struct Light {
    vec4 position_type;    // xyz = position (view space), w = light type
    vec4 color_intensity;  // xyz = color, w = intensity
    vec4 direction_range;  // xyz = direction (view space), w = range
    vec4 spot_angles;      // x = inner cos, y = outer cos
};

layout (std140) uniform Lights {
    vec4 lightCount;       // x = count, yzw unused
    Light lights[16];
};

// Constants
const float PI = 3.14159265359;
const int POINT_LIGHT = 0;
const int DIRECTIONAL_LIGHT = 1;
const int SPOT_LIGHT = 2;

// G-Buffer unpacking functions
vec3 unpackAlbedo(uint packedData)
{
    // Extract RGB 9:9:9 bits - matches packing order: R(0-8), G(9-17), B(18-26)
    uint r = packedData & 0x1FFu;         // bits 0-8 (R component)
    uint g = (packedData >> 9) & 0x1FFu;  // bits 9-17 (G component)  
    uint b = (packedData >> 18) & 0x1FFu; // bits 18-26 (B component)
    
    return vec3(float(r), float(g), float(b)) / 511.0;
}

bool unpackShadingModel(uint packedData)
{
    // Extract shading model bit (bit 27)
    return ((packedData >> 27) & 0x1u) == 1u;
}

vec3 unpackNormal(uint packedData)
{
    // Extract RGB 11:10:11 format
    uint x = packedData & 0x7FFu;          // 11 bits
    uint y = (packedData >> 11) & 0x3FFu;  // 10 bits
    uint z = (packedData >> 21) & 0x7FFu;  // 11 bits
    
    vec3 normal = vec3(
        float(x) / 2047.0,
        float(y) / 1023.0,
        float(z) / 2047.0
    );
    
    // Convert from [0,1] to [-1,1]
    return normalize(normal * 2.0 - 1.0);
}

vec3 unpackEmissive(uint packedData)
{
    if (packedData == 0u) {
        return vec3(0.0);
    }
    
    uint r = packedData & 0x1FFu;
    uint g = (packedData >> 9) & 0x1FFu;
    uint b = (packedData >> 18) & 0x1FFu;
    uint e = (packedData >> 27) & 0x1Fu;

    vec3 rgb = vec3(r, g, b) / 511.0;
    float scale = exp2(float(int(e) - 15)); 
    return rgb * scale;
}

void unpackMaterialProperties(uint packedData, out float metallic, out float roughness, out float ao, out float normalStrength)
{
    // Extract packedData 8:8:8:8 format
    metallic = float(packedData & 0xFFu) / 255.0;
    roughness = float((packedData >> 8) & 0xFFu) / 255.0;
    ao = float((packedData >> 16) & 0xFFu) / 255.0;
    normalStrength = float((packedData >> 24) & 0xFFu) / 255.0;
}

vec2 unpackMotionVectors(uint packedData)
{
    // Extract 2×16 bits motion vectors
    uint x = packedData & 0xFFFFu;        // bits 0-15
    uint y = (packedData >> 16) & 0xFFFFu; // bits 16-31
    
    vec2 motion = vec2(float(x), float(y)) / 65535.0;
    
    // Convert from [0,1] back to [-1,1] range
    return motion * 2.0 - 1.0;
}

// Reconstruct world position from depth
vec3 reconstructWorldPosition(vec2 texCoord, float depth)
{
    // Convert to NDC
    vec4 ndc = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    
    // Transform to view space
    vec4 viewPos = invProjection * ndc;
    viewPos /= viewPos.w;
    
    // Transform to world space
    vec4 worldPos = invView * viewPos;
    
    return worldPos.xyz;
}

// PBR Functions
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Calculate light attenuation and spot effect
float calculateAttenuation(int lightIndex, vec3 fragPosView, out vec3 lightDir)
{
    int lightType = int(lights[lightIndex].position_type.w);
    vec3 lightPosView = lights[lightIndex].position_type.xyz;
    float range = lights[lightIndex].direction_range.w;
    
    float attenuation = 1.0;
    
    if (lightType == DIRECTIONAL_LIGHT) {
        // Direction is stored directly in view space
        lightDir = normalize(-lights[lightIndex].direction_range.xyz);
        attenuation = 1.0;
    } else {
        // Point or spot: direction from light to fragment
        lightDir = normalize(lightPosView - fragPosView);
        float distance = length(lightPosView - fragPosView);
        
        // Attenuation
        float linearTerm = 0.045;
        float quadraticTerm = 0.0075;
        attenuation = 1.0 / (1.0 + linearTerm * distance + quadraticTerm * distance * distance);
        
        // Range fade
        if (distance > range) {
            float fadeDistance = range * 0.1;
            float fadeStart = range - fadeDistance;
            if (distance > fadeStart) {
                float fadeFactor = 1.0 - (distance - fadeStart) / fadeDistance;
                attenuation *= max(fadeFactor, 0.0);
            } else {
                attenuation = 0.0;
            }
        }
        
        // Spot cone
        if (lightType == SPOT_LIGHT) {
            vec3 spotDir = normalize(lights[lightIndex].direction_range.xyz);
            float cosAngle = dot(-lightDir, spotDir);
            float innerCos = lights[lightIndex].spot_angles.x;
            float outerCos = lights[lightIndex].spot_angles.y;
            
            float spotFactor = clamp((cosAngle - outerCos) / (innerCos - outerCos), 0.0, 1.0);
            attenuation *= spotFactor;
        }
    }
    
    return attenuation;
}

// Blinn-Phong shading
vec3 calculateBlinnPhong(int lightIndex, vec3 normal, vec3 viewDir, vec3 fragPosView, 
                        vec3 albedo, float ksIntensity, float shininess)
{
    vec3 lightDir;
    float attenuation = calculateAttenuation(lightIndex, fragPosView, lightDir);
    
    if (attenuation <= 0.0) return vec3(0.0);
    
    vec3 lightColor = lights[lightIndex].color_intensity.xyz * lights[lightIndex].color_intensity.w;
    
    vec3 materialKd = albedo;
    vec3 materialKs = vec3(ksIntensity);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * materialKd;
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = (shininess <= 0.0) ? 0.0 : pow(max(dot(normal, halfwayDir), 0.0), shininess * 256.0);
    vec3 specular = spec * lightColor * materialKs;
    
    return (diffuse + specular) * attenuation;
}

// PBR shading
vec3 calculatePBR(int lightIndex, vec3 normal, vec3 viewDir, vec3 fragPosView, 
                 vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3 lightDir;
    float attenuation = calculateAttenuation(lightIndex, fragPosView, lightDir);
    
    if (attenuation <= 0.0) return vec3(0.0);
    
    vec3 lightColor = lights[lightIndex].color_intensity.xyz * lights[lightIndex].color_intensity.w;
    vec3 radiance = lightColor * attenuation;
    
    vec3 H = normalize(viewDir + lightDir);
    roughness = clamp(roughness, 0.05, 1.0);
    
    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main()
{    
    // Sample G-Buffer data
    uvec3 gBuffer0 = texture(u_GBuffer0, TexCoord).rgb;
    uvec2 gBuffer1 = texture(u_GBuffer1, TexCoord).rg;
    float depth = texture(u_GBufferDepth, TexCoord).r;
    
    // Early exit for background pixels
    if (depth >= 1.0) {
        FragColor = vec4(0.2f,0.3f,0.3f,1.0f);
        return;
    }
    
    // Unpack G-Buffer data
    vec3 albedo = unpackAlbedo(gBuffer0.r);
    vec3 normal = unpackNormal(gBuffer0.g);
    vec3 emissive = unpackEmissive(gBuffer0.b);

    float metallic, roughness, ao, normalStrength;
    unpackMaterialProperties(gBuffer1.r, metallic, roughness, ao, normalStrength);

    // Reconstruct world position
    vec3 worldPos = reconstructWorldPosition(TexCoord, depth);

    // Convert to view space for lighting calculations
    vec4 viewPos4 = view * vec4(worldPos, 1.0);
    vec3 fragPosView = viewPos4.xyz / viewPos4.w;

    // Convert normal to view space
    vec3 normalView = mat3(view) * normal;
    normalView = normalize(normalView);

    // View direction in view space (towards camera)
    vec3 viewDir = normalize(-fragPosView);

    int numLights = int(lightCount.x);

    // Shading model selection
    bool useBlinnPhong = (u_ShadingMode == 1);

    vec3 result = vec3(0.0);

    if (useBlinnPhong) {
        // Ambient
        vec3 ambient = vec3(0.2) * 0.1 * albedo * ao;
        result += ambient;

        // Blinn-Phong lighting
        for (int i = 0; i < numLights && i < 16; ++i) {
            float shininess = (1.0 - roughness) * 128.0;
            result += calculateBlinnPhong(i, normalView, viewDir, fragPosView, albedo, 1.0, shininess);
        }

        // Emissive
        result += emissive;
    } else {
        // PBR ambient
        vec3 ambient = vec3(0.08) * albedo * ao;
        result += ambient;

        // PBR lighting
        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        for (int i = 0; i < numLights && i < 16; ++i) {
            result += calculatePBR(i, normalView, viewDir, fragPosView, albedo, metallic, roughness, F0);
        }

        // Energy compensation for rough surfaces
        if (roughness > 0.7) {
            result *= mix(1.0, 1.4, (roughness - 0.7) / 0.3);
        }

        // Emissive
        result += emissive;
    }

    // Tone mapping (ACES approximation)
    vec3 a = 2.51 * result;
    vec3 b = 0.03 + result;
    vec3 c = 2.43 * result + 0.59;
    vec3 d = 0.14 + result;
    result = clamp((a * b) / (c * d), 0.0, 1.0);

    // Gamma correction
    result = pow(result, vec3(1.0/2.2));

    FragColor = vec4(result, 1.0);
}