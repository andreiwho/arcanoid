#version 460 core

out vec4 color;

in vec2 vs_position;

void main(void)
{
    float radiusFactor = 20.0;
    vec2 center = vec2(0.0,0.0);
    float col = 1.0 - abs(radiusFactor * distance(center, vs_position));
    vec3 color_tmp = vec3(1.0, 0.5, 0.0);
    color = vec4(color_tmp * col, col + 0.1);
}