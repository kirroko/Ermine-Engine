#version 460 core

layout(location = 0) in vec3 aPosition;

uniform mat4 model;         

void main()
{
    // transform vertex from model -> world -> light clip space
    gl_Position = model * vec4(aPosition, 1.0);
}