#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D u_LightingTexture;
uniform int u_Pass; // 1 = extract bright, 2 = blur horizontal, 3 = blur vertical, 4 = combine

uniform float u_BloomThreshold = 1.0;
uniform float u_BloomIntensity = 0.8;
uniform float u_BloomRadius = 1.0;

// Gaussian blur weights for 5-tap kernel
const float weights[5] = float[](0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

vec3 extractBrightColor(vec3 color)
{
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float weight = smoothstep(u_BloomThreshold, u_BloomThreshold + 0.1, luminance);
    return color * weight;
}

vec3 gaussianBlur(sampler2D tex, vec2 texCoord, vec2 direction)
{
    vec3 result = texture(tex, texCoord).rgb * weights[0];
    
    for(int i = 1; i < 5; ++i)
    {
        vec2 offset = direction * float(i) * u_BloomRadius;
        result += texture(tex, texCoord + offset).rgb * weights[i];
        result += texture(tex, texCoord - offset).rgb * weights[i];
    }
    
    return result;
}

void main()
{
    vec2 texelSize = 1.0 / textureSize(u_LightingTexture, 0);
    
    if(u_Pass == 1) // Extract bright colors
    {
        vec3 color = texture(u_LightingTexture, TexCoord).rgb;
        vec3 brightColor = extractBrightColor(color);
        FragColor = vec4(brightColor, 1.0);
    }
    else if(u_Pass == 2) // Horizontal blur
    {
        vec3 blurred = gaussianBlur(u_LightingTexture, TexCoord, vec2(texelSize.x, 0.0));
        FragColor = vec4(blurred, 1.0);
    }
    else if(u_Pass == 3) // Vertical blur
    {
        vec3 blurred = gaussianBlur(u_LightingTexture, TexCoord, vec2(0.0, texelSize.y));
        FragColor = vec4(blurred, 1.0);
    }
    else // Pass through
    {
        FragColor = texture(u_LightingTexture, TexCoord);
    }
}