#pragma once

#include <GL/gl3w.h>
#include <stdlib.h>

struct Texture
{
    GLuint id = 0;
    int width = 0, height = 0;
};

struct Shader
{
    GLuint id = 0;
};

struct Level
{
    int* tiles = nullptr;
    int width = 0, height = 0;
};

Texture LoadTexture(const char* filename);
Shader LoadShader(const char* vertexFilename, const char* fragmentFilename);

Level LoadLevel(const char* filename);
void DestroyLevel(Level& level);
