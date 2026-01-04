#version 330 core

in vec3 fragmentColor;
in vec2 UV;

out vec4 color;

uniform sampler2D textureSampler;

void main() {
    vec4 texColor = texture(textureSampler, UV);
    color = texColor * vec4(fragmentColor, 1.0);
}