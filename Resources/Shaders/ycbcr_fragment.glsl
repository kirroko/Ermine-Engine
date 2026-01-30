/* Start Header ************************************************************************/
/*!
\file       ycbcr_fragment.glsl
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       30/01/2026
\brief      Fragment shader for YCbCr to RGB conversion using BT.601 color matrix.
            Used for MPEG1 video playback with PL_MPEG library.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

// YCbCr plane textures (single channel each)
uniform sampler2D u_TextureY;   // Luminance plane
uniform sampler2D u_TextureCb;  // Chroma blue plane
uniform sampler2D u_TextureCr;  // Chroma red plane

void main()
{
    // Sample YCbCr values from separate textures
    float y  = texture(u_TextureY, TexCoord).r;
    float cb = texture(u_TextureCb, TexCoord).r;
    float cr = texture(u_TextureCr, TexCoord).r;

    // BT.601 YCbCr to RGB conversion
    // Y is in range [0, 1], Cb and Cr are in range [0, 1] but represent [-0.5, 0.5]
    vec3 rgb;
    rgb.r = y + 1.402 * (cr - 0.5);
    rgb.g = y - 0.344136 * (cb - 0.5) - 0.714136 * (cr - 0.5);
    rgb.b = y + 1.772 * (cb - 0.5);

    // Clamp to valid range
    rgb = clamp(rgb, 0.0, 1.0);

    FragColor = vec4(rgb, 1.0);
}
