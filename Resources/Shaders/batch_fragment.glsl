#version 460

in vec2 TexCoord;
in vec3 LightIntensity;
//in vec4 TintColour;

out vec4 fragColor;

uniform sampler2D ourTexture;

void main() {
    vec4 texSample = texture(ourTexture, TexCoord);
    //fragColor = texSample * vec4(LightIntensity, 1.0) * TintColour;
    fragColor = texSample * vec4(LightIntensity, 1.0);
}