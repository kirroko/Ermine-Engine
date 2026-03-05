#version 460 core
#extension GL_ARB_bindless_texture : require

const uint MAT_FLAG_ALBEDO_MAP = 1u << 0u;

layout(std430, binding = 5) restrict readonly buffer TextureArrayBlock {
    uvec2 textureHandles[];
};

in vec2 vBaseUV;
flat in vec4 vAlbedo;
flat in vec3 vEmissive;
flat in float vEmissiveIntensity;
flat in uint vTextureFlags;
flat in int vAlbedoMapIndex;
flat in float vFillAmount;
in float vFillCoord;
in vec2 vFillScrollDirUV;

out vec4 FragColor;

uniform float u_Time;
uniform float u_FillScrollSpeed = 0.1;

void main()
{
    if (vFillCoord > clamp(vFillAmount, 0.0, 1.0)) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float dirLenSq = dot(vFillScrollDirUV, vFillScrollDirUV);
    vec2 scrollDir = (dirLenSq > 1e-8) ? (vFillScrollDirUV * inversesqrt(dirLenSq)) : vec2(0.0);
    // Subtract UV offset so perceived texture motion travels toward fill direction.
    vec2 uv = vBaseUV - (scrollDir * u_FillScrollSpeed * u_Time);

    vec4 albedo = vAlbedo;
    if ((vTextureFlags & MAT_FLAG_ALBEDO_MAP) != 0u && vAlbedoMapIndex >= 0) {
        albedo *= texture(sampler2D(textureHandles[vAlbedoMapIndex]), uv);
    }

    vec3 emissive = (vEmissive * vEmissiveIntensity) * albedo.rgb;
    FragColor = vec4(albedo.rgb + emissive, albedo.a);
}
