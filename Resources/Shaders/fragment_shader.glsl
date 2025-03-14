#version 410

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture0;

// Basic lighting parameters
uniform vec3 lightPos = vec3(5.0,5.0,5.0);
uniform vec3 lightColor = vec3(1.0,1.0,1.0);
uniform vec3 viewPos = vec3(0.0,0.0,5.0);

void main()
{
    // Basic texture sampling
    vec4 texColor = texture(texture0,TexCoord);

    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // Combine lighting components
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;

    FragColor = vec4(result, texColor.a);
}