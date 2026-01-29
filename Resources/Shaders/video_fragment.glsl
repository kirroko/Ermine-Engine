#version 450 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D tex_y;
uniform sampler2D tex_cb;
uniform sampler2D tex_cr;

void main()
{
    float y = texture(tex_y, vTexCoord).r;
    float cb = texture(tex_cb, vTexCoord).r - 0.5;
    float cr = texture(tex_cr, vTexCoord).r - 0.5;

    vec3 rgb;
    rgb.r = y + 1.402 * cr;
    rgb.g = y - 0.344136 * cb - 0.714136 * cr;
    rgb.b = y + 1.772 * cb;

    FragColor = vec4(clamp(rgb, 0.0, 1.0), 1.0);
}
