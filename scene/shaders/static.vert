#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 color;

uniform mat4 MVP;

out vec3 worldPosition;
out vec3 worldNormal;
out vec2 fragTexCoord;
out vec3 fragColor;

void main() {
    gl_Position = MVP * vec4(position, 1.0);

    worldPosition = position;
    worldNormal = normal;

    fragTexCoord = texCoord;
    fragColor = color;
}