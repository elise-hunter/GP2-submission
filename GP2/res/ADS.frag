// ADS fragment shader implementing ambient + diffuse Phong lighting.
// Specular is intentionally omitted for this exercise.

#version 330 core

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

out vec4 FragColor;

void main()
{
    // A small constant ambient term prevents surfaces facing away from the
    // light from rendering as pure black.
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // Re-normalise because interpolation across the triangle can change the length of the normal.
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    // Clamp to zero so back-facing surfaces don't subtract light.
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    FragColor = vec4((ambient + diffuse) * objectColor, 1.0);
}
