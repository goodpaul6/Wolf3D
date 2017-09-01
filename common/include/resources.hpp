#pragma once

#include <gl3w.h>
#include <stdlib.h>
#include <stb_truetype.h>

#define ET_MASK(etype) (1 << (etype))

static const int FONT_BITMAP_WIDTH = 512;
static const int FONT_BITMAP_HEIGHT = 512;

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

struct Font
{
    Texture texture;
    float height = 0.0f;
    stbtt_fontinfo info;
    mutable stbtt_bakedchar glyphs[96];
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
        //  *---------*
        //  b         a + b
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

Font LoadFont(const char* filename, float height);
void DestroyFont(Font& font);

// TODO: Implement functions to destroy textures and shaders

Level LoadLevel(const char* filename);
void DestroyLevel(Level& level);
