#version 330 core

in vec2 f_texCoord;
out vec3 color;

uniform sampler2D tex;
uniform vec3 tint;

void main()
{
    vec4 texCol = texture(tex, f_texCoord);

    if(texCol.r < 0.1)
        discard;
    color = tint;
}
