#pragma once

#include <GL/gl3w.h>

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

Mesh CreateMesh(int vertexCount, const Vertex* vertices, int indexCount, const ushort* indices);
Mesh CreatePlaneMesh();
Mesh CreateLevelMesh(const Level& level, const Texture& texture);
void Draw(const Mesh& mesh);
void DestroyMesh(Mesh& mesh);
