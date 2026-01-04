#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;

uniform mat4 MVP;

out vec3 fragmentColor;
out vec2 UV;

void main() {
    gl_Position = MVP * vec4(vertexPosition, 1.0);
    fragmentColor = vertexColor;
    UV = vertexUV;
}