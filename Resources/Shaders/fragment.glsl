#version 460

in vec3 color;
in vec2 TexCoord;

out vec4 fragColor;

uniform sampler2D ourTexture;

void main()
{
    fragColor = texture(ourTexture, TexCoord);
}