#version 330 core

layout (location = 0) in vec2 v_pos;
layout (location = 1) in vec2 v_texCoord;

out vec2 f_texCoord;

uniform vec2 viewSize;

void main() 
{
    f_texCoord = v_texCoord;
    gl_Position = vec4(2.0 * (v_pos.x / viewSize.x) - 1.0, 
                       1.0 - 2.0 * (v_pos.y / viewSize.y), 
                       0.0, 1.0);
}

