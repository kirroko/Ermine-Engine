#version 460 core
#extension GL_ARB_bindless_texture : require

// Inputs from vertex shader
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 ViewPos;
in vec3 Tangent;
in vec3 Bitangent;
flat in uint vMaterialIndex;
flat in vec3 vModelCenter;  // Model center in world space
flat in vec3 vCameraPos;    // Camera position in world space

// Output
out vec4 FragColor;

// Material structure matching your engine
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

layout(std430, binding = 5) restrict readonly buffer TextureArrayBlock {
    uvec2 textureHandles[];
};

// Uniforms
uniform float u_Time;

// Volumetric cone parameters
// These are RELATIVE to the cone's actual geometry scale
uniform float volumetricDensity = 0.8;
uniform float heightFalloff = 0.5;
uniform float edgeSoftness = 2.0;
uniform float noiseScale = 1.5;
uniform float noiseSpeed = 0.3;
uniform int numSteps = 48;

// Intensity multiplier for HDR glow
uniform float intensityMultiplier = 3.0;

// Simple 3D noise function
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    
    return mix(
        mix(mix(hash(i + vec3(0, 0, 0)), hash(i + vec3(1, 0, 0)), f.x),
            mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
        mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x),
            mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y),
        f.z
    );
}

// Fractal Brownian Motion for detailed noise
float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    
    return value;
}

// Ray-sphere intersection (bounding volume)
bool intersectSphere(vec3 ro, vec3 rd, vec3 center, float radius, out float t0, out float t1) {
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h < 0.0) return false;
    h = sqrt(h);
    t0 = -b - h;
    t1 = -b + h;
    return true;
}

// Quaternion to rotation matrix conversion
mat3 quaternionToMatrix(vec4 q) {
    float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
    
    float xx = qx * qx;
    float yy = qy * qy;
    float zz = qz * qz;
    float xy = qx * qy;
    float xz = qx * qz;
    float yz = qy * qz;
    float wx = qw * qx;
    float wy = qw * qy;
    float wz = qw * qz;
    
    return mat3(
        1.0 - 2.0 * (yy + zz), 2.0 * (xy + wz), 2.0 * (xz - wy),
        2.0 * (xy - wz), 1.0 - 2.0 * (xx + zz), 2.0 * (yz + wx),
        2.0 * (xz + wy), 2.0 * (yz - wx), 1.0 - 2.0 * (xx + yy)
    );
}

// Calculate volumetric density at a point inside the cone
// The cone is defined by its geometry - we sample relative to model space
float sampleConeDensity(vec3 worldPos, vec3 modelCenter, mat3 invRotation, vec3 invScale) {
    // Transform world position to model-local space
    vec3 localPos = worldPos - modelCenter;
    
    // Apply inverse rotation to get into model's local coordinate system
    localPos = invRotation * localPos;
    
    // Apply inverse scale
    localPos = localPos / invScale;
    
    // Now we're in the cone's original local space where:
    // - Apex is at (0, height, 0)
    // - Base is at (0, 0, 0)
    // - Cone grows along +Y axis
    
    // Cone parameters in unscaled local space
    float coneHeight = 3.5; // Half of primitive.size.y (7.0) / 2 from scale
    float baseRadius = 1.9; // Half of primitive.size.x (3.8) / 2 from scale
    
    // Height along cone axis (Y-axis in local space)
    float height = localPos.y;
    
    // Normalized height (0 at base, 1 at apex)
    float normalizedHeight = clamp(height / coneHeight, 0.0, 1.0);
    
    // Outside height bounds
    if (normalizedHeight < 0.0 || normalizedHeight > 1.0) {
        return 0.0;
    }
    
    // Radius at this height (cone tapers from base to apex)
    float radiusAtHeight = baseRadius * (1.0 - normalizedHeight);
    
    if (radiusAtHeight < 0.001) return 0.0;
    
    // Distance from cone axis (Y-axis) in XZ plane
    float distFromAxis = length(localPos.xz);
    
    // Normalized distance (0 at axis, 1 at edge)
    float normalizedDist = distFromAxis / radiusAtHeight;
    
    if (normalizedDist > 1.0) {
        return 0.0;
    }
    
    // Base density with radial falloff (soft edges)
    float radialFalloff = pow(1.0 - normalizedDist, edgeSoftness);
    
    // Height-based intensity (stronger at apex, like a spotlight beam)
    float heightIntensity = pow(normalizedHeight, heightFalloff);
    
    // Add noise variation for organic look (use world space for consistent noise)
    vec3 noiseCoord = worldPos * noiseScale + vec3(0.0, u_Time * noiseSpeed, 0.0);
    float noiseValue = fbm(noiseCoord);
    float noiseMod = 0.6 + 0.4 * noiseValue;
    
    return radialFalloff * heightIntensity * noiseMod * volumetricDensity;
}

void main()
{
    MaterialData material = materials[vMaterialIndex];
    
    // Extract transform from the cone's actual transform
    // From scene: rotation quaternion (0.025327, -0.038989, 0.331335, 0.942367)
    // Scale: (2.5, 1.3, 2.5)
    vec4 rotation = vec4(0.025326933711767198, -0.03898869827389717, 0.33133459091186526, 0.9423671364784241);
    vec3 scale = vec3(2.5, 1.3, 2.5);
    
    // Create rotation matrix from quaternion
    mat3 rotationMatrix = quaternionToMatrix(rotation);
    
    // Inverse rotation (transpose for orthogonal matrix)
    mat3 invRotation = transpose(rotationMatrix);
    
    // Inverse scale
    vec3 invScale = vec3(1.0 / scale.x, 1.0 / scale.y, 1.0 / scale.z);
    
    // Setup ray
    vec3 rayOrigin = vCameraPos;
    vec3 rayDir = normalize(FragPos - vCameraPos);
    
    // Bounding sphere should account for rotated and scaled cone
    // Maximum extent is roughly max(scale) * max(primitive size)
    float boundingRadius = max(scale.x, max(scale.y, scale.z)) * 4.0;
    
    // Intersect with bounding volume
    float tNear, tFar;
    if (!intersectSphere(rayOrigin, rayDir, vModelCenter, boundingRadius, tNear, tFar)) {
        discard;
    }
    
    tNear = max(tNear, 0.0);
    if (tNear >= tFar) discard;
    
    // Ray march through volume
    float stepSize = (tFar - tNear) / float(numSteps);
    vec3 accumulatedColor = vec3(0.0);
    float accumulatedAlpha = 0.0;
    
    for (int i = 0; i < numSteps; i++) {
        if (accumulatedAlpha > 0.98) break;
        
        float t = tNear + (float(i) + 0.5) * stepSize;
        vec3 samplePos = rayOrigin + rayDir * t;
        
        // Sample density at this position (with proper transform)
        float density = sampleConeDensity(samplePos, vModelCenter, invRotation, invScale);
        
        if (density > 0.01) {
            // Calculate sample color (material color + emissive)
            vec3 sampleColor = material.albedo.rgb + material.emissive * material.emissiveIntensity;
            
            // Apply intensity multiplier for HDR glow
            sampleColor *= intensityMultiplier;
            
            // Simple height-based lighting (brighter at top in LOCAL space)
            vec3 localPos = samplePos - vModelCenter;
            localPos = invRotation * localPos;
            localPos = localPos / invScale;
            float heightFactor = clamp(localPos.y / 3.5, 0.0, 1.0);
            sampleColor = mix(sampleColor * 0.5, sampleColor * 1.5, heightFactor);
            
            // Calculate sample opacity
            float sampleAlpha = density * stepSize * 2.0;
            sampleAlpha = clamp(sampleAlpha, 0.0, 1.0);
            
            // Front-to-back alpha blending
            accumulatedColor += sampleColor * sampleAlpha * (1.0 - accumulatedAlpha);
            accumulatedAlpha += sampleAlpha * (1.0 - accumulatedAlpha);
        }
    }
    
    // Apply material alpha
    float materialAlpha = material.albedo.a;
    accumulatedAlpha *= materialAlpha;
    
    // Output final color
    FragColor = vec4(accumulatedColor, accumulatedAlpha);
    
    // Discard fully transparent fragments
    if (FragColor.a < 0.01) {
        discard;
    }
}