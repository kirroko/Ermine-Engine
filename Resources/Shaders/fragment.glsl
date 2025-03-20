#version 460

in vec2 TexCoord;
in vec3 LightIntensity;

out vec4 fragColor;

uniform sampler2D ourTexture;

void main()
{
    fragColor = texture(ourTexture, TexCoord) * vec4(LightIntensity, 1.0);
}