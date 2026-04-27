#version 400 core

// xy = position on screen, zw = position in the font texture
layout(location = 0) in vec4 vertex;

out vec2 texCoord;

uniform mat4 projection; // orthographic, screen pixels

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    texCoord    = vertex.zw;
}
