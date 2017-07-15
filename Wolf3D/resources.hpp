#pragma once

#include <GL/gl3w.h>
#include <stdlib.h>

static const int MAX_LOAD_MESH_VERTICES = 500;

struct Mesh;

struct Texture
{
    GLuint id = 0;
    int width = 0, height = 0;
};

struct Shader
{
    GLuint id = 0;
};

enum EntityType : int
{
    ET_PLAYER,
    ET_DOOR,
    ET_ENEMY,
    ET_COUNT
};

struct EntityInfo
{
    EntityType type;
    float x, y, z;
    
    union 
    {
        int dir;        // ET_DOOR

        struct          // ET_ENEMY
        {
            int health;     
            float speed;
        };
    };
};

struct Level
{
    int* tiles = nullptr;
    int width = 0, height = 0;

    // Number of entities of each type
    int entityCount[ET_COUNT]; 
    EntityInfo* entities[ET_COUNT];
};

Texture LoadTexture(const char* filename);
Shader LoadShader(const char* vertexFilename, const char* fragmentFilename);
Mesh LoadMesh(const char* filename);

Level LoadLevel(const char* filename);
void DestroyLevel(Level& level);
