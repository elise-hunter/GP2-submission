// Vertex shader. Positions each vertex on screen and sets up the normal mapping
// data needed by the fragment shader.

#version 400 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 tangent;

out vec2 texCoord0;
out vec3 Normal0;
out vec3 WorldPos0;
out mat3 TBN;

uniform mat4 transform;
uniform mat4 model;
uniform vec2 uvOffset;

void main()
{
    gl_Position = transform * vec4(position, 1.0);
    texCoord0   = texCoord + uvOffset;
    WorldPos0   = (model * vec4(position, 1.0)).xyz;

    vec3 worldNormal  = normalize(mat3(model) * normal);
    vec3 worldTangent = normalize(mat3(model) * tangent);

    // Re-straighten the tangent relative to the normal in case scaling has skewed them apart.
    worldTangent = normalize(worldTangent - dot(worldTangent, worldNormal) * worldNormal);

    vec3 worldBitangent = cross(worldNormal, worldTangent);

    TBN     = mat3(worldTangent, worldBitangent, worldNormal);
    Normal0 = worldNormal;
}
