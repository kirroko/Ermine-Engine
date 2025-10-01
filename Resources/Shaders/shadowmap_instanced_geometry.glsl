#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// Input from vertex shader
flat in int v_Layer[];

void main()
{
    // Set the layer for this triangle
    gl_Layer = v_Layer[0];

    // Pass through all vertices
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}