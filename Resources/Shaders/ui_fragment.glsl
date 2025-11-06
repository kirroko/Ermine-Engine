#version 450 core

// Input from vertex shader
in vec4 vColor;

// Output color
out vec4 FragColor;

void main()
{
    FragColor = vColor;
}
