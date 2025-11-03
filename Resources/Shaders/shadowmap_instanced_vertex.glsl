#version 460 core

#ifdef GL_NV_viewport_array2
#extension GL_NV_viewport_array2 : enable
#endif

#ifdef GL_AMD_vertex_shader_layer
#extension GL_AMD_vertex_shader_layer : enable
#endif

#ifdef GL_ARB_shader_viewport_layer_array
#extension GL_ARB_shader_viewport_layer_array : enable
#endif

const int MAX_LIGHTS = 32;
const int NUM_CASCADES = 4;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;      // Not used for shadows, but needed for attribute layout
layout(location = 2) in vec2 aTexCoord;    // Not used for shadows, but needed for attribute layout
layout(location = 3) in vec3 aTangent;     // Not used for shadows, but needed for attribute layout

// Skinning attributes
layout(location = 4) in ivec4 aBoneIDs;
layout(location = 5) in vec4 aWeights;

// Per-vertex uniforms
uniform mat4 model;

// Skinning uniforms
uniform bool u_UseSkinning;
uniform mat4 u_BoneMatrices[128];

// Light structure
struct Light {
    vec4 position_type;    // xyz = position (view space), w = light type
    vec4 color_intensity;  // xyz = color, w = intensity
    vec4 direction_range;  // xyz = direction (view space), w = range
    vec4 spot_angles_castshadows_startOffset; // x = inner angle (cos), y = outer angle (cos), z = cast shadows (bool), w = shadow map index or 0 if no shadows
    mat4 lightSpaceMatrix[NUM_CASCADES]; // Light view-projection matrices for cascaded shadow maps
    vec4 splitDepths[(NUM_CASCADES + 3) / 4]; // Split depths for cascaded shadow maps
};

layout (std140, binding = 1) uniform LightsUBO {
    vec4 lightCount;
    Light lights[MAX_LIGHTS]; // Fixed-size array required for UBO
};

// Per-frame uniforms - avoid additional SSBOs
uniform int u_ActiveShadowLights[16];    // Indices of shadow-casting directional lights

void main()
{
    // Apply skinning transformation if enabled
    vec4 skinnedPos = vec4(aPosition, 1.0);
    
    if (u_UseSkinning) {
        mat4 boneTransform =
            u_BoneMatrices[aBoneIDs[0]] * aWeights[0] +
            u_BoneMatrices[aBoneIDs[1]] * aWeights[1] +
            u_BoneMatrices[aBoneIDs[2]] * aWeights[2] +
            u_BoneMatrices[aBoneIDs[3]] * aWeights[3];
        
        skinnedPos = boneTransform * vec4(aPosition, 1.0);
    }

    // Calculate light and cascade from gl_InstanceID
    // Instance layout: light0_cascade0, light0_cascade1, ..., light0_cascade3, light1_cascade0, ...
    int cascadeIndex = gl_InstanceID % NUM_CASCADES;
    int lightArrayIndex = gl_InstanceID / NUM_CASCADES;

    // Get the actual light index from the active shadow lights array
    int lightIndex = u_ActiveShadowLights[lightArrayIndex];

    // Get the light data
    Light light = lights[lightIndex];

    // Calculate target layer: startOffset + cascadeIndex
    int startOffset = int(light.spot_angles_castshadows_startOffset.w);
    int targetLayer = startOffset + cascadeIndex;

    // Transform vertex to light space using the appropriate cascade matrix
    gl_Position = light.lightSpaceMatrix[cascadeIndex] * model * skinnedPos;

    // Pass layer to fragment shader (for gl_Layer assignment if needed)
    gl_Layer = targetLayer;
}