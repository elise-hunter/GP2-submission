#version 400 core

layout(location = 0) in vec3 position;

out float ndcY;
out float ndcX;

void main()
{
    gl_Position = vec4(position, 1.0);
    ndcY = position.y;
    ndcX = position.x;
}
