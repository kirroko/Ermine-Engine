#version 460

in vec2 vTexCoord;
in vec4 vColor;
in float vLife;
in vec3 vWorldPos;

out vec4 FragColor;

uniform float u_Time;
uniform int u_SparkleShape;  // 0 = soft circle, 1 = star, 2 = diamond
uniform int u_RenderMode;    // 0 = glow, 1 = smoke
uniform float u_SmokeOpacity;
uniform float u_SmokeSoftness;
uniform float u_SmokeNoiseScale;
uniform float u_SmokeDistortScale;
uniform float u_SmokeDistortStrength;
uniform float u_SmokePuffScale;
uniform float u_SmokePuffStrength;
uniform float u_SmokeDepthFade;
uniform vec2 u_ScreenSize;
uniform sampler3D u_SmokeNoise;
uniform sampler3D u_SmokeDistort;
uniform sampler3D u_SmokePuff;
uniform sampler2D u_SceneDepth;

// Soft circular gradient
float softCircle(vec2 uv) {
    float dist = length(uv - 0.5) * 2.0;
    return 1.0 - smoothstep(0.0, 1.0, dist);
}

// Star/sparkle shape
float starShape(vec2 uv, int points) {
    vec2 centered = uv - 0.5;
    float dist = length(centered);
    float angle = atan(centered.y, centered.x);

    // Create star points
    float star = cos(angle * float(points)) * 0.5 + 0.5;
    star = pow(star, 2.0);

    // Combine with radial falloff
    float radial = 1.0 - smoothstep(0.0, 0.5, dist);
    float core = 1.0 - smoothstep(0.0, 0.15, dist);

    return max(radial * star, core);
}

// Diamond/cross shape
float diamondShape(vec2 uv) {
    vec2 centered = abs(uv - 0.5) * 2.0;

    // Diamond
    float diamond = 1.0 - (centered.x + centered.y);
    diamond = smoothstep(0.0, 0.3, diamond);

    // Cross rays
    float crossH = smoothstep(0.1, 0.0, centered.y) * (1.0 - centered.x);
    float crossV = smoothstep(0.1, 0.0, centered.x) * (1.0 - centered.y);

    return max(diamond, max(crossH, crossV) * 0.5);
}

void main() {
    if (u_RenderMode == 1) {
        vec3 baseUV = vWorldPos * u_SmokeNoiseScale + vec3(0.0, u_Time * 0.05, 0.0);
        vec3 distortUV = vWorldPos * u_SmokeDistortScale + vec3(0.0, u_Time * 0.08, 0.0);
        vec3 distort = texture(u_SmokeDistort, distortUV).rrr * 2.0 - 1.0;
        baseUV += distort * u_SmokeDistortStrength;

        float noise = texture(u_SmokeNoise, baseUV).r;
        float puff = texture(u_SmokePuff, vWorldPos * u_SmokePuffScale).r;
        float density = mix(noise, puff, u_SmokePuffStrength);
        density = smoothstep(0.2, 1.0, density);

        float circle = softCircle(vTexCoord);
        float alpha = circle * density * (1.0 - vLife);
        alpha = smoothstep(0.0, max(0.001, u_SmokeSoftness), alpha) * u_SmokeOpacity;

        // Soft particles: fade when intersecting scene depth
        if (u_ScreenSize.x > 0.0 && u_ScreenSize.y > 0.0) {
            vec2 uv = gl_FragCoord.xy / u_ScreenSize;
            float sceneDepth = texture(u_SceneDepth, uv).r;
            float depthDiff = sceneDepth - gl_FragCoord.z;
            float depthFade = clamp(depthDiff * u_SmokeDepthFade, 0.0, 1.0);
            alpha *= depthFade;
        }

        if (alpha < 0.01) discard;
        vec3 color = mix(vColor.rgb * 0.2, vColor.rgb, 1.0 - density);
        FragColor = vec4(color, alpha);
        return;
    }

    // Choose shape
    float shape;
    if (u_SparkleShape == 1) {
        shape = starShape(vTexCoord, 4);
    } else if (u_SparkleShape == 2) {
        shape = diamondShape(vTexCoord);
    } else {
        shape = softCircle(vTexCoord);
    }

    // Discard fully transparent pixels
    if (shape < 0.01 || vColor.a < 0.01) {
        discard;
    }

    // Apply color with HDR intensity for bloom
    vec3 color = vColor.rgb * 2.0;  // HDR boost

    // Add extra glow in center
    float centerGlow = 1.0 - smoothstep(0.0, 0.3, length(vTexCoord - 0.5));
    color += vColor.rgb * centerGlow * 1.5;

    // Final alpha
    float alpha = shape * vColor.a;

    FragColor = vec4(color, alpha);
}
