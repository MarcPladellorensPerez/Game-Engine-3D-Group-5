#version 460 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform bool useTexture = false;
uniform bool useLighting = true;
uniform vec3 objectColor = vec3(0.8, 0.8, 0.8);
uniform vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));

void main()
{
    vec4 baseColor = useTexture ? texture(texture_diffuse1, TexCoords) : vec4(objectColor, 1.0);

    if (useLighting)
    {
        vec3 norm = normalize(Normal);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 ambient = 0.4 * vec3(1.0);
        vec3 diffuse = diff * vec3(0.8);
        vec3 result = (ambient + diffuse) * baseColor.rgb;
        FragColor = vec4(result, baseColor.a);
    }
    else
    {
        FragColor = baseColor;
    }
}
