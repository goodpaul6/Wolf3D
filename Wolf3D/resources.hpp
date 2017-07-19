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
    struct Edge
    {
        // Edge vertices in grid coordinates
        int x1, z1;
        int x2, z2;
        // Edges cannot be oriented (just 
        // create two edges at different y
        // positions and connect them)
        int y;
    };

    struct Connection
    {
        // Edge 1, Edge 2
        int e1, e2;
        // Tile to use for this connection
        int tile; 
    };

    int edgeCount;
    Edge* edges;

    int connectionCount;
    Connection* connections;

    // Number of entities of each type
    int entityCount[ET_COUNT]; 
    EntityInfo* entities[ET_COUNT];
};

Texture LoadTexture(const char* filename);
Shader LoadShader(const char* vertexFilename, const char* fragmentFilename);
Mesh LoadMesh(const char* filename);

Level LoadLevel(const char* filename);
void DestroyLevel(Level& level);
