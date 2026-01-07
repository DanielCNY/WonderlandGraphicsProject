#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec4 vertexJoints;
layout(location = 4) in vec4 vertexWeights;

out vec3 worldPosition;
out vec3 worldNormal;

uniform mat4 MVP;
uniform mat4 jointMatrices[100];

void main() {
    mat4 skin = vertexWeights.x * jointMatrices[int(vertexJoints.x)] +
        vertexWeights.y * jointMatrices[int(vertexJoints.y)] +
        vertexWeights.z * jointMatrices[int(vertexJoints.z)] +
        vertexWeights.w * jointMatrices[int(vertexJoints.w)];

    vec4 position = skin * vec4(vertexPosition, 1.0);
    vec3 normal = mat3(skin) * vertexNormal;

    gl_Position =  MVP * position;

    worldPosition = position.xyz;
    worldNormal = normalize(normal);
}
