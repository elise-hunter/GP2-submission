// ADS lighting vertex shader. Passes each vertex's world position and
// corrected normal to the fragment shader for lighting.

#version 330 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 2) in vec3 VertexNormal;

uniform mat4 model;
uniform mat3 normalMatrix;
uniform mat4 transform;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    FragPos = vec3(model * vec4(VertexPosition, 1.0));

    // The normal matrix stops normals from getting stretched when the object is scaled unevenly.
    Normal = normalMatrix * VertexNormal;

    // w = 1.0 so it gets translated (it's a position, not a direction).
    gl_Position = transform * vec4(VertexPosition, 1.0);
}
