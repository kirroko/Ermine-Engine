#version 450 core

in vec2 TexCoord;
out vec4 FragColor;

// Input textures
uniform sampler2D u_LightingTexture;
uniform sampler2D u_BloomTexture;
uniform sampler2D u_SceneDepth;  

// Post-processing toggles
uniform int u_Vignette = 1;
uniform int u_FXAA = 1;
uniform int u_ToneMapping = 1;
uniform int u_GammaCorrection = 1;
uniform int u_Bloom = 1;
uniform int u_SkyboxIsHDR = 0;

// Post-processing parameters
uniform float u_Exposure = 1.0;
uniform float u_Contrast = 1.0;
uniform float u_Saturation = 1.0;
uniform float u_Gamma = 2.2;
uniform float u_VignetteIntensity = 0.3;
uniform float u_VignetteRadius = 0.8;
uniform float u_BloomStrength = 0.04;

// FXAA parameters
uniform float u_FXAASpanMax = 8.0;
uniform float u_FXAAReduceMin = 1.0/128.0;
uniform float u_FXAAReduceMul = 1.0/8.0;

// Tone mapping functions
vec3 reinhardToneMapping(vec3 color)
{
    return color / (color + vec3(1.0));
}

vec3 exposureToneMapping(vec3 color, float exposure)
{
    return vec3(1.0) - exp(-color * exposure);
}

vec3 acesToneMapping(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

// Gamma correction
vec3 gammaCorrection(vec3 color, float gamma)
{
    return pow(color, vec3(1.0 / gamma));
}

// Vignette effect
vec3 applyVignette(vec3 color, vec2 texCoord)
{
    vec2 uv = texCoord * 2.0 - 1.0;
    float vignette = 1.0 - dot(uv, uv) * u_VignetteIntensity;
    vignette = smoothstep(0.0, u_VignetteRadius, vignette);
    return color * vignette;
}

// FXAA implementation
vec3 applyFXAA(sampler2D tex, vec2 texCoord)
{
    vec2 inverseVP = 1.0 / textureSize(tex, 0);
    
    vec3 rgbNW = texture(tex, texCoord + vec2(-1.0, -1.0) * inverseVP).rgb;
    vec3 rgbNE = texture(tex, texCoord + vec2(1.0, -1.0) * inverseVP).rgb;
    vec3 rgbSW = texture(tex, texCoord + vec2(-1.0, 1.0) * inverseVP).rgb;
    vec3 rgbSE = texture(tex, texCoord + vec2(1.0, 1.0) * inverseVP).rgb;
    vec3 rgbM = texture(tex, texCoord).rgb;
    
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM = dot(rgbM, luma);
    
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * u_FXAAReduceMul), u_FXAAReduceMin);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(vec2(u_FXAASpanMax), max(vec2(-u_FXAASpanMax), dir * rcpDirMin)) * inverseVP;
    
    vec3 rgbA = 0.5 * (
        texture(tex, texCoord + dir * (1.0/3.0 - 0.5)).rgb +
        texture(tex, texCoord + dir * (2.0/3.0 - 0.5)).rgb);
    
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, texCoord + dir * -0.5).rgb +
        texture(tex, texCoord + dir * 0.5).rgb);
    
    float lumaB = dot(rgbB, luma);
    
    if((lumaB < lumaMin) || (lumaB > lumaMax))
        return rgbA;
    else
        return rgbB;
}

// Color grading functions
vec3 adjustContrast(vec3 color, float contrast)
{
    return (color - 0.5) * contrast + 0.5;
}

vec3 adjustSaturation(vec3 color, float saturation)
{
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return mix(vec3(luminance), color, saturation);
}

void main()
{
    vec3 color;
    float sceneDepth = texture(u_SceneDepth, TexCoord).r;
    bool isSky = sceneDepth >= 1.0;

    
    if(u_FXAA == 1)
    {
        color = applyFXAA(u_LightingTexture, TexCoord);
    }
    else
    {
        color = texture(u_LightingTexture, TexCoord).rgb;
    }
    
    if(u_Bloom == 1)
    {
        vec3 bloomColor = texture(u_BloomTexture, TexCoord).rgb;
        color += bloomColor * u_BloomStrength;
    }
    
    if(!isSky || u_SkyboxIsHDR == 1)
    {
        if(u_ToneMapping == 1)
        {
            // Using ACES tone mapping for better results
            color = acesToneMapping(color * u_Exposure);
        }
        
        if(u_GammaCorrection == 1)
        {
            color = gammaCorrection(color, u_Gamma);
        }
    }
    
    if(u_Vignette == 1)
    {
        color = applyVignette(color, TexCoord);
    }

    color = adjustContrast(color, u_Contrast);

    color = adjustSaturation(color, u_Saturation);
    
    FragColor = vec4(color, 1.0);
}