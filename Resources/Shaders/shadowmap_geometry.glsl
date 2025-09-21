
#version 460 core

const int NUM_CASCADES = 4;
const int SHADOW_MAX_LAYERS = 32;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3 * SHADOW_MAX_LAYERS) out;

// Light structure
struct Light {
    vec4 position_type;
    vec4 color_intensity;
    vec4 direction_range;
    vec4 spot_angles_castshadows_startOffset;
    mat4 lightSpaceMatrix[NUM_CASCADES];
    vec4 splitDepths;
};

layout (std140) uniform Lights {
    vec4 lightCount;
    Light lights[16];
};

void main()
{
    int numLights = int(lightCount.x);

    for (int l = 0; l < numLights; ++l)
    {
        // Check if this light casts shadows and is directional
        int lightType = int(lights[l].position_type.w);
        bool castsShadows = lights[l].spot_angles_castshadows_startOffset.z > 0.5;
        
        if (castsShadows && lightType == 1) // DIRECTIONAL_LIGHT
        {
            int startOffset = int(lights[l].spot_angles_castshadows_startOffset.w);

            for (int c = 0; c < NUM_CASCADES; ++c)
            {
                int targetLayer = startOffset + c;
                
                // Set the layer for this cascade
                gl_Layer = targetLayer;

                // Emit triangle for this cascade
                for (int i = 0; i < 3; ++i)
                {
                    gl_Position = lights[l].lightSpaceMatrix[c] * gl_in[i].gl_Position;
                    EmitVertex();
                }
                EndPrimitive();
            }
        }
    }
}