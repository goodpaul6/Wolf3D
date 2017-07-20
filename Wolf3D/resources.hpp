#pragma once

#include <GL/gl3w.h>
#include <stdlib.h>

static const float LEVEL_SCALE_FACTOR = 2.0f;
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
    ET_PAINTING,
    ET_BOXCOLLIDER,
    ET_COUNT
};

struct EntityInfo
{
    EntityType type;
    float x, y, z;
    
    union 
    {
        int dir;        // ET_DOOR, ET_PAINTING

        struct          // ET_ENEMY
        {
            int health;     
            float speed;
        };

        struct
        {
            float minx, miny, minz;
            float maxx, maxy, maxz;
        };
    };
};

struct Level
{
    struct Plane
    {
        // All of these are in half-grid coordinates (LEVEL_SCALE_FACTOR / 2)
        // o = origin 
        // a = first plane vertex
        // b = second plane vertex
        // 
        //  o         a
        //  *---------*
        //  |         |
        //  |         |
        //  |         |
        //  *---------* b
        //  b-(a-o)
        int o[3], a[3], b[3];
        int tile;
    };

    int planeCount = 0;
    Plane* planes = nullptr;

    // Number of entities of each type
    int entityCount[ET_COUNT] = {0};
    EntityInfo* entities[ET_COUNT] = {0};
};

Texture LoadTexture(const char* filename);
Shader LoadShader(const char* vertexFilename, const char* fragmentFilename);
Mesh LoadMesh(const char* filename);

Level LoadLevel(const char* filename);
void DestroyLevel(Level& level);
