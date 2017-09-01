#version 330 core

layout (location = 0) in vec2 v_pos;

uniform vec2 viewSize;

void main() 
{
    gl_Position = vec4(2.0 * (v_pos.x / viewSize.x) - 1.0, 
                       1.0 - 2.0 * (v_pos.y / viewSize.y), 
                       0.0, 1.0);
}
