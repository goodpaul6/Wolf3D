#version 330 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec2 v_texCoord;

out vec2 f_texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    f_texCoord = v_texCoord;
    gl_Position = proj * view * model * vec4(v_pos, 1.0);
}
