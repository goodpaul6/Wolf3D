#pragma once

#include <GL/gl3w.h>
#include <stdlib.h>

static const int ENTITY_TYPE_MAX_LENGTH = 32;

struct EntityInfo
{
    float x = 0, y = 0, z = 0;
};

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

    // Number of entity types
    int typeCount = 0;
    char** types = nullptr;

    // Number of entities of a particular type
    int* entityCount = nullptr; 

    EntityInfo** entities = nullptr;
};

Texture LoadTexture(const char* filename);
Shader LoadShader(const char* vertexFilename, const char* fragmentFilename);

Level LoadLevel(const char* filename);
void DestroyLevel(Level& level);
