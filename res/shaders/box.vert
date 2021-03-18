#version 460 core

layout(location = 0) in vec2 position;

out vec2 vs_position;

uniform mat4 projectionMatrix;

void main(void)
{
    vs_position = position;
    gl_Position = projectionMatrix * vec4(position, 0.0, 1.0);
}

