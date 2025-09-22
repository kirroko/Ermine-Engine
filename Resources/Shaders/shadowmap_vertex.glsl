#version 460 core

layout(location = 0) in vec3 aPosition;

uniform mat4 model;         
uniform mat4 u_LightViewProj; 

void main()
{
    // transform vertex from model -> world -> light clip space
    gl_Position = u_LightViewProj * model * vec4(aPosition, 1.0);
}