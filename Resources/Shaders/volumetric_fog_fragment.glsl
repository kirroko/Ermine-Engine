#version 460 core
#extension GL_ARB_bindless_texture : require

const int MAX_LIGHTS = 32;
const int NUM_CASCADES = 4;

// Inputs from vertex shader
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 ViewPos;
in vec3 Tangent;
in vec3 Bitangent;
flat in uint vMaterialIndex;
flat in vec3 vCameraPos;    // Camera position in world space
flat in vec3 vModelCenter;  // Model center in world space

// Output
out vec4 FragColor;

// Material structure
struct MaterialData {
    vec4 albedo;
    float metallic;
    float roughness;
    float ao;
    float normalStrength;
    vec3 emissive;
    float emissiveIntensity;
    int shadingModel;
    uint textureFlags;
    float _pad0;
    float _pad1;
    vec2 uvScale;
    vec2 uvOffset;
    int albedoMapIndex;
    int normalMapIndex;
    int roughnessMapIndex;
    int metallicMapIndex;
    int aoMapIndex;
    int emissiveMapIndex;
    int _pad2;
    int _pad3;
};

layout(std430, binding = 3) restrict readonly buffer MaterialBlock {
    MaterialData materials[];
};

// Light structure (matching lighting_fragment.glsl)
struct Light {
    vec4 position_type;    // xyz = position (world space), w = light type
    vec4 color_intensity;  // xyz = color, w = intensity
    vec4 direction_range;  // xyz = direction (world space), w = range
    vec4 spot_angles_castshadows_startOffset; // x = inner angle (cos), y = outer angle (cos), z = cast shadows (bool), w = shadow map index or 0 if no shadows
    mat4 lightSpaceMatrix[NUM_CASCADES]; // Light view-projection matrices for cascaded shadow maps
    vec4 splitDepths[(NUM_CASCADES + 3) / 4]; // Split depths for cascaded shadow maps
};

layout (std140, binding = 1) uniform LightsUBO {
    vec4 lightCount;
    Light lights[MAX_LIGHTS]; // Fixed-size array required for UBO
};

// Uniforms
uniform float u_Time;

// Volumetric fog parameters
uniform float fogDensity = 0.3;           // Base density of the fog
uniform float fogAbsorption = 0.5;        // How much light fog absorbs
uniform float fogScattering = 0.8;        // How much light fog scatters
uniform float noiseScale = 0.5;           // Scale of noise turbulence
uniform float noiseSpeed = 0.1;           // Speed of noise animation
uniform int numSteps = 64;                // Ray marching steps
uniform float lightScatterPower = 2.0;    // Controls spotlight beam visibility
uniform float ambientFogStrength = 0.05;  // Ambient fog contribution

// Light type constants
const int POINT_LIGHT = 0;
const int DIRECTIONAL_LIGHT = 1;
const int SPOT_LIGHT = 2;

const float PI = 3.14159265359;

// 3D noise for fog variation
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(mix(hash(p + vec3(0,0,0)), hash(p + vec3(1,0,0)), f.x),
            mix(hash(p + vec3(0,1,0)), hash(p + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash(p + vec3(0,0,1)), hash(p + vec3(1,0,1)), f.x),
            mix(hash(p + vec3(0,1,1)), hash(p + vec3(1,1,1)), f.x), f.y),
        f.z);
}

// Fractal Brownian Motion for detailed fog
float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 3; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

// Ray-AABB intersection (for mesh bounding box)
bool intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax, out float tMin, out float tMax) {
    vec3 invDir = 1.0 / rayDir;
    vec3 t0 = (boxMin - rayOrigin) * invDir;
    vec3 t1 = (boxMax - rayOrigin) * invDir;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    tMin = max(max(tmin.x, tmin.y), tmin.z);
    tMax = min(min(tmax.x, tmax.y), tmax.z);

    return tMax >= tMin && tMax >= 0.0;
}

// Sample fog density at a point
float sampleFogDensity(vec3 worldPos) {
    // Base density
    float density = fogDensity;

    // Add noise variation for volumetric turbulence
    vec3 noiseCoord = worldPos * noiseScale + vec3(u_Time * noiseSpeed * 0.5, u_Time * noiseSpeed, 0.0);
    float noiseValue = fbm(noiseCoord);

    // Modulate density with noise (0.3 to 1.0 range)
    density *= 0.3 + 0.7 * noiseValue;

    // Optional: add height-based falloff for more realistic atmospheric fog
    float heightFalloff = exp(-max(0.0, worldPos.y - vModelCenter.y) * 0.1);
    density *= mix(1.0, heightFalloff, 0.5);

    return density;
}

// Calculate spotlight contribution to fog at a point
vec3 calculateSpotlightFog(int lightIndex, vec3 worldPos, vec3 rayDir) {
    // Get light properties
    vec3 lightPos = lights[lightIndex].position_type.xyz;
    vec3 lightColor = lights[lightIndex].color_intensity.xyz;
    float lightIntensity = lights[lightIndex].color_intensity.w;
    vec3 spotDir = normalize(lights[lightIndex].direction_range.xyz);
    float range = lights[lightIndex].direction_range.w;
    float innerCos = lights[lightIndex].spot_angles_castshadows_startOffset.x;
    float outerCos = lights[lightIndex].spot_angles_castshadows_startOffset.y;

    // Vector from light to sample point
    vec3 lightToPoint = worldPos - lightPos;
    float distance = length(lightToPoint);
    vec3 lightDir = lightToPoint / distance;

    // Check if point is within spotlight range
    if (distance > range) {
        return vec3(0.0);
    }

    // Spotlight cone attenuation
    float cosAngle = dot(lightDir, spotDir);
    if (cosAngle < outerCos) {
        return vec3(0.0);
    }

    // Smooth falloff from inner to outer cone
    float spotFactor = smoothstep(outerCos, innerCos, cosAngle);

    // Distance attenuation (inverse square with smooth falloff)
    float attenuation = 1.0 / (1.0 + distance * distance * 0.01);

    // Range fade near the edge
    float fadeDistance = range * 0.2;
    float fadeStart = range - fadeDistance;
    attenuation *= smoothstep(range, fadeStart, distance);

    // In-scattering: how much light scatters toward camera
    // Henyey-Greenstein phase function approximation
    float viewAlignment = dot(-rayDir, -lightDir);
    float phase = (1.0 - lightScatterPower * lightScatterPower) /
                  pow(1.0 + lightScatterPower * lightScatterPower - 2.0 * lightScatterPower * viewAlignment, 1.5);
    phase = max(0.0, phase);

    // Additional spotlight beam visibility boost
    float beamVisibility = pow(spotFactor, 0.5); // Makes the beam more visible

    // Combine all factors
    vec3 scatteredLight = lightColor * lightIntensity * attenuation * spotFactor * phase * beamVisibility * fogScattering;

    return scatteredLight;
}

// Calculate point light contribution to fog at a point
vec3 calculatePointLightFog(int lightIndex, vec3 worldPos, vec3 rayDir) {
    // Get light properties
    vec3 lightPos = lights[lightIndex].position_type.xyz;
    vec3 lightColor = lights[lightIndex].color_intensity.xyz;
    float lightIntensity = lights[lightIndex].color_intensity.w;
    float range = lights[lightIndex].direction_range.w;

    // Vector from light to sample point
    vec3 lightToPoint = worldPos - lightPos;
    float distance = length(lightToPoint);
    vec3 lightDir = lightToPoint / distance;

    // Check if point is within light range
    if (distance > range) {
        return vec3(0.0);
    }

    // Distance attenuation
    float attenuation = 1.0 / (1.0 + distance * distance * 0.01);

    // Range fade
    float fadeDistance = range * 0.2;
    float fadeStart = range - fadeDistance;
    attenuation *= smoothstep(range, fadeStart, distance);

    // In-scattering phase function (simplified)
    float viewAlignment = dot(-rayDir, -lightDir);
    float phase = 0.5 + 0.5 * viewAlignment;

    // Combine factors
    vec3 scatteredLight = lightColor * lightIntensity * attenuation * phase * fogScattering;

    return scatteredLight;
}

void main()
{
    MaterialData material = materials[vMaterialIndex];

    // Setup ray - march from camera to fragment position (mesh surface)
    vec3 rayOrigin = vCameraPos;
    vec3 rayDir = normalize(FragPos - vCameraPos);

    // Calculate the march distance - from camera to the mesh surface
    float tNear = 0.0;
    float tFar = length(FragPos - vCameraPos);

    // If camera is inside the mesh, start from camera position
    if (tFar < 0.01) {
        discard;
    }

    // Raymarch through volume
    float stepSize = (tFar - tNear) / float(numSteps);
    vec3 accumulatedColor = vec3(0.0);
    float transmittance = 1.0; // How much light passes through

    int numLights = int(lightCount.x);

    for (int i = 0; i < numSteps; i++) {
        if (transmittance < 0.01) break; // Early exit if fog is opaque

        float t = tNear + (float(i) + 0.5) * stepSize;
        vec3 samplePos = rayOrigin + rayDir * t;

        // Sample fog density at this position
        float density = sampleFogDensity(samplePos);

        if (density > 0.001) {
            // Calculate extinction (how much light is absorbed/scattered)
            float extinction = density * fogAbsorption * stepSize;
            float sampleTransmittance = exp(-extinction);

            // Accumulate light from all lights
            vec3 lightContribution = vec3(0.0);

            // Add ambient fog color
            vec3 ambientColor = material.albedo.rgb * ambientFogStrength;
            lightContribution += ambientColor;

            // Add contribution from each light
            for (int j = 0; j < numLights; j++) {
                int lightType = int(lights[j].position_type.w);

                if (lightType == SPOT_LIGHT) {
                    lightContribution += calculateSpotlightFog(j, samplePos, rayDir);
                } else if (lightType == POINT_LIGHT) {
                    lightContribution += calculatePointLightFog(j, samplePos, rayDir);
                }
                // Directional lights can be added here if needed
            }

            // In-scattering: light scattered into view direction
            vec3 inscatter = lightContribution * density * stepSize;

            // Accumulate color (front-to-back compositing)
            accumulatedColor += inscatter * transmittance;

            // Update transmittance
            transmittance *= sampleTransmittance;
        }
    }

    // Final alpha based on how much light was absorbed
    float alpha = 1.0 - transmittance;

    // Apply material alpha multiplier
    alpha *= material.albedo.a;

    // Add material emissive for glowing fog effect
    if (material.emissiveIntensity > 0.0) {
        accumulatedColor += material.emissive * material.emissiveIntensity * alpha;
    }

    FragColor = vec4(accumulatedColor, alpha);

    // Discard fully transparent fragments
    if (FragColor.a < 0.001) {
        discard;
    }
}
