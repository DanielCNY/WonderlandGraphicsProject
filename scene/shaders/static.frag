#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 fragTexCoord;
in vec3 fragColor;

out vec3 finalColor;

uniform vec3 lightPosition;
uniform vec3 lightIntensity;
uniform sampler2D textureSampler;

void main()
{
    vec3 lightDir = lightPosition - worldPosition;
    float lightDist = dot(lightDir, lightDir);
    lightDir = normalize(lightDir);
    vec3 v = lightIntensity * clamp(dot(lightDir, worldNormal), 0.0, 1.0) / lightDist;

    v = v / (1.0 + v);

    v = max(v, vec3(0.2));

    vec3 texColor = texture(textureSampler, fragTexCoord).rgb;
    vec3 baseColor = (length(texColor) > 0.1) ? texColor : fragColor;

    finalColor = baseColor * pow(v, vec3(1.0 / 2.2));
}