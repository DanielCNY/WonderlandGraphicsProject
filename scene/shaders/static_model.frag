#version 330 core

in vec3 fragNormal;
in vec2 fragTexCoord;

uniform sampler2D textureSampler;

out vec4 outColor;

void main() {
     vec4 texColor = texture(textureSampler, fragTexCoord);

     vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
     vec3 normal = normalize(fragNormal);
     float diffuse = max(dot(normal, lightDir), 0.3);
     float ambient = 0.7;

     outColor = vec4(texColor.rgb * (ambient + diffuse), 1.0);
}