#version 330 core

in vec3 fragColor;
in vec3 fragNormal;
in vec2 fragTexCoord;

out vec4 outColor;

void main() {
    // Simple lighting with fixed light direction
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(fragNormal);
    float diffuse = max(dot(normal, lightDir), 0.2);

    // Use color from vertex or texture coordinate for simple coloring
    vec3 color = fragColor;
    if (color == vec3(0.0)) {
        // If no color provided, use a nice green for trees
        color = vec3(0.2, 0.6, 0.3);
    }

    outColor = vec4(color * diffuse, 1.0);
}