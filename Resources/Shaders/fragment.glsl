#version 460

in vec2 TexCoord;
in vec3 LightIntensity;

out vec4 fragColor;

uniform sampler2D ourTexture;

float near = 0.1f;
float far = 100.0f;

float linearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0; // Back to NDC
	return (2.0 * near * far) / (far + near - z * (far - near)); // From NDC to world space
}

void main()
{
    fragColor = texture(ourTexture, TexCoord) * vec4(LightIntensity, 1.0);
//	fragColor = vec4(vec3(linearizeDepth(gl_FragCoord.z)) / far,1.0);
}