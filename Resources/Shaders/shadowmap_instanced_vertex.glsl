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

// Draw info structure matching CPU-side DrawInfo (std430 layout)
// Total size: 160 bytes (must match C++ DrawInfo in DrawCommands.h)
struct DrawInfo {
    mat4 modelMatrix;           // 64 bytes (offset 0-63) - Model transformation matrix
    vec3 aabbMin;               // 12 bytes (offset 64-75) - AABB minimum bounds
    uint materialIndex;         // 4 bytes (offset 76-79) - Index into material SSBO
    vec3 aabbMax;               // 12 bytes (offset 80-91) - AABB maximum bounds
    uint entityID;              // 4 bytes (offset 92-95) - Entity ID
    uint flags;                 // 4 bytes (offset 96-99) - Flags (bit 0: useSkinning)
    uint boneTransformOffset;   // 4 bytes (offset 100-103) - Starting index in skeletal SSBO
    uint _pad0;                 // 4 bytes (offset 104-107) - Padding
    uint _pad1;                 // 4 bytes (offset 108-111) - Padding to 16-byte boundary
    vec4 normalMatrixCol0;      // 16 bytes (offset 112-127) - Normal matrix column 0 (xyz used)
    vec4 normalMatrixCol1;      // 16 bytes (offset 128-143) - Normal matrix column 1 (xyz used)
    vec4 normalMatrixCol2;      // 16 bytes (offset 144-159) - Normal matrix column 2 (xyz used)
    
};

// SSBO binding for indirect rendering DrawInfo
layout(std430, binding = 1) restrict readonly buffer DrawInfoBuffer {
    DrawInfo drawInfos[];
};

// Skeletal animation bone transforms SSBO (Binding 2)
layout(std430, binding = 2) restrict readonly buffer BoneTransformBuffer {
    mat4 boneTransforms[]; // All bone transforms for all entities
};

// Base draw ID offset for multi-batch indirect rendering
uniform uint baseDrawID;

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
    // Get draw info from SSBO using gl_DrawID (indirect rendering)
    DrawInfo drawInfo = drawInfos[baseDrawID + gl_DrawID];
    mat4 modelMatrix = drawInfo.modelMatrix;
    bool useSkinning = (drawInfo.flags & 1u) != 0u;

    // Calculate skinned position
    vec4 skinnedPos = vec4(aPosition, 1.0);

    if (useSkinning) {
        // Get bone offset for this entity from DrawInfo
        uint boneOffset = drawInfo.boneTransformOffset;

        // Calculate final bone transform using weighted blend
        mat4 boneTransform =
            boneTransforms[boneOffset + aBoneIDs[0]] * aWeights[0] +
            boneTransforms[boneOffset + aBoneIDs[1]] * aWeights[1] +
            boneTransforms[boneOffset + aBoneIDs[2]] * aWeights[2] +
            boneTransforms[boneOffset + aBoneIDs[3]] * aWeights[3];

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
    gl_Position = light.lightSpaceMatrix[cascadeIndex] * modelMatrix * skinnedPos;

    // Pass layer to fragment shader (for gl_Layer assignment if needed)
    gl_Layer = targetLayer;
}
