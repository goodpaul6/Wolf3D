#pragma once

#include <GL/gl3w.h>
#include <SDL.h>

#include "types.hpp"

struct Level;
struct Texture;

struct Vertex
{
    float x, y, z;
    float u, v;
};

struct Mesh
{
    int vertexCount, indexCount;

    Vertex* vertices;
    ushort* indices;

    GLuint vertexArray;
    GLuint buffers[2];
};

void GetTileUV(const Texture& texture, int tile, float& u1, float& v1, float& u2, float& v2);

Mesh CreateMesh(int vertexCount, const Vertex* vertices, int indexCount, const ushort* indices);
Mesh CreatePlaneMesh(float u1 = 0, float v1 = 0, float u2 = 1, float v2 = 1);
Mesh CreateLevelMesh(const Level& level, const Texture& texture);

// Changes the plane's uv coordinates to show a particular frame in a texture
void PlaneShowFrame(Mesh& mesh, const Texture& texture, int fw, int fh, int frame);

void DestroyMesh(Mesh& mesh);

void Draw(const Mesh& mesh);

