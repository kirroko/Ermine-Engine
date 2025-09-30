#version 460 core

const int NUM_CASCADES = 4;

layout(location = 0) in vec3 aPosition;

// Per-vertex uniforms
uniform mat4 model;

// Light structure
struct Light {
    vec4 position_type;
    vec4 color_intensity;
    vec4 direction_range;
    vec4 spot_angles_castshadows_startOffset;
    mat4 lightSpaceMatrix[NUM_CASCADES];
    vec4 splitDepths[(NUM_CASCADES+3)/4];
};

layout (std430, binding = 1) restrict readonly buffer LightsSSBO {
    vec4 lightCount;
    Light lights[];
};

// Per-frame uniforms - avoid additional SSBOs
uniform int u_ActiveShadowLights[16];    // Indices of shadow-casting directional lights

// Output for fragment shader
flat out int v_Layer;

void main()
{
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
    gl_Position = light.lightSpaceMatrix[cascadeIndex] * model * vec4(aPosition, 1.0);

    // Pass layer to fragment shader (for gl_Layer assignment if needed)
    v_Layer = targetLayer;
}