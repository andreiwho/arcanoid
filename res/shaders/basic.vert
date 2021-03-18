#version 460 core

layout(location = 0) in vec2 position;

uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;

void main(void)
{
    gl_Position = projectionMatrix * modelMatrix * vec4(position, 0.0, 1.0);
}