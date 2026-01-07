#version 330 core

in vec3 worldPosition;
in vec3 worldNormal; 

out vec3 finalColor;

uniform vec3 lightPosition;
uniform vec3 lightIntensity;

void main()
{
	vec3 lightDir = lightPosition - worldPosition;
	float lightDist = dot(lightDir, lightDir);
	lightDir = normalize(lightDir);
	vec3 v = lightIntensity * clamp(dot(lightDir, worldNormal), 0.0, 1.0) / lightDist;

	v = v / (1.0 + v);

    vec3 baseColor = vec3(0.7, 0.0, 0.0);
	finalColor = baseColor * pow(v, vec3(1.0 / 2.2));
}
