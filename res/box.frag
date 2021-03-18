#version 460 core

out vec4 color;

in vec2 vs_position;

void main(void)
{
    color = vec4(vs_position.x + 0.3, 0.5, vs_position.y - 0.5, 1.0);
}