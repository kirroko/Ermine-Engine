#version 460
#extension GL_ARB_bindless_texture : require

// ========== EFFECT SETTINGS (EDIT THESE) ==========
// Set to 1 to make orbs visible even on fully transparent areas
// Set to 0 to respect material transparency
#define IGNORE_TRANSPARENCY 1

// Orb behavior
#define ORB_MODE 0  // 0 = Converging (charging), 1 = Expelling (release)

// Visual quality (higher = better quality but slower)
#define RAY_MARCH_STEPS 32       // Number of depth samples (16-48)
#define ORB_DENSITY 12.0         // Grid density (8.0-20.0)
#define ANIMATION_SPEED 0.3      // Speed of orb movement

// Orb appearance
#define ORB_SIZE_MIN 0.03        // Smallest orb size
#define ORB_SIZE_MAX 0.15        // Largest orb size
#define ORB_GLOW 3.0             // Glow intensity multiplier

// Material texture flag bits
const uint MAT_FLAG_ALBEDO_MAP    = 1u << 0u;
const uint MAT_FLAG_NORMAL_MAP    = 1u << 1u;
const uint MAT_FLAG_ROUGHNESS_MAP = 1u << 2u;
const uint MAT_FLAG_METALLIC_MAP  = 1u << 3u;
const uint MAT_FLAG_AO_MAP        = 1u << 4u;
const uint MAT_FLAG_EMISSIVE_MAP  = 1u << 5u;

// Inputs from vertex shader
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 ViewPos;
in vec3 Tangent;
in vec3 Bitangent;
flat in uint vMaterialIndex;

out vec4 FragColor;

// Material structure (112 bytes, matches C++ MaterialSSBO)
struct MaterialData {
    vec4 albedo;                // 16 bytes (0-15)
    float metallic;             // 4 bytes (16-19)
    float roughness;            // 4 bytes (20-23)
    float ao;                   // 4 bytes (24-27)
    float normalStrength;       // 4 bytes (28-31)

    vec3 emissive;              // 12 bytes (32-43)
    float emissiveIntensity;    // 4 bytes (44-47)

    int shadingModel;           // 4 bytes (48-51)
    uint textureFlags;          // 4 bytes (52-55)
    float _pad0;                // 4 bytes (56-59)
    float _pad1;                // 4 bytes (60-63)

    vec2 uvScale;               // 8 bytes (64-71)
    vec2 uvOffset;              // 8 bytes (72-79)

    // Texture Array Indices
    int albedoMapIndex;         // 4 bytes (80-83)
    int normalMapIndex;         // 4 bytes (84-87)
    int roughnessMapIndex;      // 4 bytes (88-91)
    int metallicMapIndex;       // 4 bytes (92-95)

    int aoMapIndex;             // 4 bytes (96-99)
    int emissiveMapIndex;       // 4 bytes (100-103)
    int _pad2;                  // 4 bytes (104-107)
    int _pad3;                  // 4 bytes (108-111)
};

// Material SSBO
layout(std430, binding = 3) restrict readonly buffer MaterialBlock {
    MaterialData materials[];
};

// Bindless texture array SSBO
layout(std430, binding = 5) restrict readonly buffer TextureArrayBlock {
    uvec2 textureHandles[];
};

// Delta time uniform for animation
uniform float u_Time;

const float PI = 3.14159265359;

// ========== 3D HASH FUNCTIONS ==========

// Hash function for 3D positions
float hash3D(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.x + p.y) * p.z);
}

// 3D hash returning vec3 for random offsets
vec3 hash3D3(vec3 p) {
    p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return fract(sin(p) * 43758.5453123);
}

// ========== 3D VOLUMETRIC ORB RENDERING ==========

// Ray march through 3D volume to render thousands of orbs converging/expelling from single center point
// Creates the illusion of looking into a sphere/cube of energy from the surface
float renderVolumetricOrbs(vec2 uv, vec2 center) {
    // Create 3D view ray - transform UV to view direction
    // This simulates looking through the surface into a 3D volume
    vec3 viewDir = normalize(vec3((uv - center) * 2.5, -1.0));

    // Ray origin starts in front of the volume
    vec3 rayOrigin = vec3((uv - center) * 1.0, 1.0);

    // Single center point in 3D space where all orbs converge/expel
    vec3 volumeCenter = vec3(0.0, 0.0, 0.0);

    float totalIntensity = 0.0;
    float time = u_Time * ANIMATION_SPEED;

    // Ray march through the volume
    for(int step = 0; step < RAY_MARCH_STEPS; step++) {
        float t = float(step) / float(RAY_MARCH_STEPS);

        // Sample position along ray (going deeper into volume)
        vec3 rayPos = rayOrigin + viewDir * t * 3.0;

        // Distance from current sample to volume center
        float distToVolumeCenter = length(rayPos - volumeCenter);

        // Generate orbs in a dense grid around this sample position
        vec3 gridPos = rayPos * ORB_DENSITY;
        vec3 cellId = floor(gridPos);
        vec3 localPos = fract(gridPos);

        // Check neighboring grid cells for orbs
        for(int x = -1; x <= 1; x++) {
            for(int y = -1; y <= 1; y++) {
                for(int z = -1; z <= 1; z++) {
                    vec3 neighborOffset = vec3(float(x), float(y), float(z));
                    vec3 cellCoord = cellId + neighborOffset;

                    // Generate unique random seed for this cell
                    vec3 cellSeed = cellCoord * 0.1;
                    float cellHash = hash3D(cellCoord);
                    vec3 cellRandom = hash3D3(cellCoord);

                    // Random position within this grid cell (0 to 1)
                    vec3 randomPosInCell = cellRandom;

                    // Actual world position of this orb's "home" location in the grid
                    vec3 orbHomePos = (cellCoord + randomPosInCell) / ORB_DENSITY;

                    // Direction from volume center to this orb's home position
                    vec3 dirFromVolumeCenter = normalize(orbHomePos - volumeCenter);
                    float distFromVolumeCenter = length(orbHomePos - volumeCenter);

                    // Each orb has unique timing offset
                    float phaseOffset = cellHash * 6.28;

                    #if ORB_MODE == 0
                        // CONVERGING: Orbs start far from center, move inward
                        float animProgress = fract(time * 0.5 + phaseOffset + distFromVolumeCenter * 0.3);

                        // Orb starts at home position, moves toward volume center
                        float startDist = distFromVolumeCenter;
                        float endDist = 0.05;  // Almost reaches center
                        float currentDist = mix(startDist, endDist, animProgress);

                        // Current orb position in 3D space
                        vec3 currentOrbPos = volumeCenter + dirFromVolumeCenter * currentDist;

                        // Orb shrinks as it approaches center
                        float orbSize = mix(ORB_SIZE_MAX, ORB_SIZE_MIN, animProgress);

                        // Fade out when reaching center
                        float fade = smoothstep(0.0, 0.1, currentDist);

                    #else
                        // EXPELLING: Orbs start at center, explode outward
                        float animProgress = fract(time * 0.5 + phaseOffset - distFromVolumeCenter * 0.3);

                        // Orb starts near center, moves to home position
                        float startDist = 0.05;
                        float endDist = distFromVolumeCenter;
                        float currentDist = mix(startDist, endDist, animProgress);

                        // Current orb position in 3D space
                        vec3 currentOrbPos = volumeCenter + dirFromVolumeCenter * currentDist;

                        // Orb grows as it moves away
                        float orbSize = mix(ORB_SIZE_MIN, ORB_SIZE_MAX, animProgress);

                        // Fade out when reaching max distance
                        float maxDist = 2.0;
                        float fade = 1.0 - smoothstep(maxDist * 0.7, maxDist, currentDist);
                    #endif

                    // Distance from ray sample point to current orb position
                    float distToOrb = length(rayPos - currentOrbPos);

                    // Render spherical orb with soft falloff
                    float orbContribution = smoothstep(orbSize * 2.0, orbSize * 0.3, distToOrb);

                    // Brightness based on progress (charging gets brighter near center)
                    float brightness = 1.0 - animProgress * 0.3;

                    // Depth fade - orbs deeper in volume are slightly dimmer
                    float depthFade = 1.0 - t * 0.4;

                    // Accumulate contribution
                    totalIntensity += orbContribution * fade * brightness * depthFade;
                }
            }
        }
    }

    // Scale and clamp final intensity
    return clamp(totalIntensity * 0.08 * ORB_GLOW, 0.0, 1.0);
}

// ========== COLOR GRADIENTS ==========

// Energy color gradient (cyan to white)
vec3 energyGradient(float t) {
    vec3 dark = vec3(0.0, 0.2, 0.4);      // Deep blue
    vec3 mid = vec3(0.0, 0.8, 1.0);       // Cyan
    vec3 bright = vec3(0.5, 1.0, 1.0);    // Light cyan
    vec3 white = vec3(1.0, 1.0, 1.0);     // White

    if(t < 0.33) return mix(dark, mid, t * 3.0);
    if(t < 0.66) return mix(mid, bright, (t - 0.33) * 3.0);
    return mix(bright, white, (t - 0.66) * 3.0);
}

// Fire color gradient (red to yellow to white)
vec3 fireGradient(float t) {
    vec3 red = vec3(1.0, 0.1, 0.0);
    vec3 orange = vec3(1.0, 0.5, 0.0);
    vec3 yellow = vec3(1.0, 1.0, 0.3);
    vec3 white = vec3(1.0, 1.0, 1.0);

    if(t < 0.33) return mix(red, orange, t * 3.0);
    if(t < 0.66) return mix(orange, yellow, (t - 0.33) * 3.0);
    return mix(yellow, white, (t - 0.66) * 3.0);
}

// Purple energy gradient
vec3 purpleGradient(float t) {
    vec3 dark = vec3(0.2, 0.0, 0.4);
    vec3 purple = vec3(0.6, 0.2, 1.0);
    vec3 pink = vec3(1.0, 0.4, 0.8);
    vec3 white = vec3(1.0, 1.0, 1.0);

    if(t < 0.33) return mix(dark, purple, t * 3.0);
    if(t < 0.66) return mix(purple, pink, (t - 0.33) * 3.0);
    return mix(pink, white, (t - 0.66) * 3.0);
}

// ========== MAIN SHADER ==========

void main()
{
    // Get material data
    MaterialData material = materials[vMaterialIndex];
    vec2 uv = TexCoord * material.uvScale + material.uvOffset;

    // Render volumetric orbs
    float orbIntensity = renderVolumetricOrbs(uv, vec2(0.5, 0.5));

    // Choose color gradient (uncomment your preference)
    vec3 orbColor = energyGradient(orbIntensity);
    // vec3 orbColor = fireGradient(orbIntensity);
    // vec3 orbColor = purpleGradient(orbIntensity);

    // Sample base texture
    vec4 albedo = material.albedo;
    if ((material.textureFlags & MAT_FLAG_ALBEDO_MAP) != 0u && material.albedoMapIndex >= 0) {
        uvec2 handle = textureHandles[material.albedoMapIndex];
        sampler2D albedoMap = sampler2D(handle);
        albedo *= texture(albedoMap, uv);
    }

    // Lighting
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(ViewPos - FragPos);

    // Rim lighting for depth
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = pow(rim, 3.0);

    // Combine colors
    vec3 baseColor = albedo.rgb * 0.1;  // Very dim base
    vec3 orbGlow = orbColor * orbIntensity * 5.0;  // Very bright orbs
    vec3 rimGlow = orbColor * rim * orbIntensity * 0.8;  // Edge glow enhanced by orb presence

    // Emissive pulsing
    float pulse = 0.85 + 0.15 * sin(u_Time * 2.5);
    vec3 emissive = material.emissive * material.emissiveIntensity * pulse;

    // Final color - emphasize orbs over base material
    vec3 finalColor = baseColor + orbGlow + rimGlow + emissive;

    // Extra bloom boost for orb areas
    finalColor *= mix(1.2, 2.5, orbIntensity);

    // Alpha calculation
    #if IGNORE_TRANSPARENCY == 1
        // Orbs always fully visible where they exist
        // Quick transition to full opacity for better visibility
        float alpha = smoothstep(0.0, 0.1, orbIntensity);
    #else
        // Blend with material transparency
        float baseAlpha = albedo.a * 0.2;
        float orbAlpha = smoothstep(0.0, 0.3, orbIntensity);
        float alpha = clamp(baseAlpha + orbAlpha, 0.0, 1.0);
    #endif

    FragColor = vec4(finalColor, alpha);
}
