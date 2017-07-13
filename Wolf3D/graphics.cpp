#include <GL/gl3w.h>
#include <stdlib.h>

#include "utils.hpp"
#include "graphics.hpp"
#include "resources.hpp"

static const ushort PLANE_INDICES[] =
{
    // Top-right triangle
    0, 1, 2,
    // Bottom-left triangle
    2, 3, 0
};

static const int TILE_WIDTH = 64;
static const int TILE_HEIGHT = 64;

void GetTileUV(const Texture& texture, int tile, float& u1, float& v1, float& u2, float& v2)
{
    int columns = texture.width / TILE_WIDTH;

    float w = (float)TILE_WIDTH / texture.width;
    float h = (float)TILE_HEIGHT / texture.height;

    u1 = (float)((tile % columns) * TILE_WIDTH) / texture.width + w;
    v1 = (float)((tile / columns) * TILE_HEIGHT) / texture.height + h;

    u2 = u1 - w;
    v2 = v1 - h;
}

Mesh CreateMesh(int vertexCount, const Vertex* vertices, int indexCount, const ushort* indices)
{
    Mesh mesh;
    
    mesh.vertexCount = vertexCount;
    mesh.indexCount = indexCount;

    mesh.vertices = (Vertex*)malloc(sizeof(Vertex) * vertexCount);
    mesh.indices = (ushort*)malloc(sizeof(ushort) * indexCount);

    memcpy(mesh.vertices, vertices, sizeof(Vertex) * vertexCount);
    memcpy(mesh.indices, indices, sizeof(ushort) * indexCount);

    glGenVertexArrays(1, &mesh.vertexArray);
    glBindVertexArray(mesh.vertexArray);

    glGenBuffers(2, mesh.buffers);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.buffers[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.buffers[1]);

    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexCount, vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ushort) * indexCount, indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    return mesh;
}

Mesh CreatePlaneMesh(float u1, float v1, float u2, float v2)
{
    const Vertex vertices[] =
    {
        // Top-left
        { -1, 1, 0, u1, v1 },
        // Top-right
        { 1, 1, 0, u2, v1 },
        // Bottom-right
        { 1, -1, 0, u2, v2 },
        // Bottom-left
        { -1, -1, 0, u1, v2 }
    };

    return CreateMesh(COUNT_OF(vertices), vertices, 
                      COUNT_OF(PLANE_INDICES), PLANE_INDICES);
}

Mesh CreateLevelMesh(const Level& level, const Texture& texture)
{
    int vertexCount = 0;
    Vertex* vertices = (Vertex*)malloc(sizeof(Vertex) * 16 * level.width * level.height); 
    
    int indexCount = 0;
	ushort* indices = (ushort*)malloc(sizeof(ushort) * 36 * level.width * level.height);

#define PUSH_FLOOR(x, y, z, u1, v1, u2, v2) do { \
    float xx = (float)(x) * 2; \
    float yy = (float)(y) * 2; \
    float zz = (float)(z) * 2; \
    vertices[vertexCount++] = { (float)xx, yy, (float)zz, (float)(u1), (float)(v1) }; \
    vertices[vertexCount++] = { (float)xx + 2, yy, (float)zz, (float)(u2), (float)(v1) }; \
    vertices[vertexCount++] = { (float)xx + 2, yy, (float)zz + 2, (float)(u2), (float)(v2) }; \
    vertices[vertexCount++] = { (float)xx, yy, (float)zz + 2, (float)(u1), (float)(v2) }; \
	ushort i = vertexCount - 4; \
    indices[indexCount++] = i; \
    indices[indexCount++] = i + 1; \
    indices[indexCount++] = i + 2; \
    indices[indexCount++] = i + 2; \
    indices[indexCount++] = i + 3; \
    indices[indexCount++] = i; \
} while(0)

#define PUSH_WALL_Z(x, z, u1, v1, u2, v2) do { \
    float xx = (float)(x) * 2; \
    float zz = (float)(z) * 2; \
    vertices[vertexCount++] = { (float)xx, 0.0f, (float)zz, (float)(u1), (float)(v1) }; \
    vertices[vertexCount++] = { (float)xx, 2.0f, (float)zz, (float)(u1), (float)(v2) }; \
    vertices[vertexCount++] = { (float)xx + 2, 2.0f, (float)zz, (float)(u2), (float)(v2) }; \
    vertices[vertexCount++] = { (float)xx + 2, 0.0f, (float)zz, (float)(u2), (float)(v1) }; \
    ushort i = vertexCount - 4; \
    indices[indexCount++] = i + 3; \
    indices[indexCount++] = i + 2; \
    indices[indexCount++] = i + 1; \
    indices[indexCount++] = i + 1; \
    indices[indexCount++] = i + 0; \
    indices[indexCount++] = i + 3; \
} while(0)

#define PUSH_WALL_X(x, z, u1, v1, u2, v2) do { \
    float xx = (float)(x) * 2; \
    float zz = (float)(z) * 2; \
    vertices[vertexCount++] = { (float)xx, 0.0f, (float)zz, (float)(u1), (float)(v1) }; \
    vertices[vertexCount++] = { (float)xx, 2.0f, (float)zz, (float)(u1), (float)(v2) }; \
    vertices[vertexCount++] = { (float)xx, 2.0f, (float)zz + 2, (float)(u2), (float)(v2) }; \
    vertices[vertexCount++] = { (float)xx, 0.0f, (float)zz + 2, (float)(u2), (float)(v1) }; \
    ushort i = vertexCount - 4; \
    indices[indexCount++] = i + 0; \
    indices[indexCount++] = i + 1; \
    indices[indexCount++] = i + 2; \
    indices[indexCount++] = i + 2; \
    indices[indexCount++] = i + 3; \
    indices[indexCount++] = i + 0; \
} while(0)

    for(int z = 0; z < level.height; ++z)
    {
        for(int x = 0; x < level.width; ++x)
        {
            int tile = level.tiles[x + z * level.width];
            
            float u1, v1, u2, v2;
            GetTileUV(texture, tile, u1, v1, u2, v2);
            
            if(tile > 0)
            {
				PUSH_WALL_X(x, z, u1, v1, u2, v2);
				PUSH_WALL_X(x + 1, z, u1, v1, u2, v2);
                PUSH_WALL_Z(x, z, u1, v1, u2, v2);
				PUSH_WALL_Z(x, z + 1, u1, v1, u2, v2);
            }
            else
            {
                PUSH_FLOOR(x, 0, z, u1, v1, u2, v2);

                // Recalculate uv's for ceiling tile
                tile = 108;
				GetTileUV(texture, tile, u1, v1, u2, v2);
                PUSH_FLOOR(x, 1, z, u1, v1, u2, v2);
            }
        }
    }

#undef PUSH_FLOOR
#undef PUSH_WALL_Z
#undef PUSH_WALL_X

    Mesh mesh = CreateMesh(vertexCount, vertices, indexCount, indices);

    free(vertices);
    free(indices);

    return mesh;
}

void DestroyMesh(Mesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vertexArray);
    glDeleteBuffers(2, mesh.buffers);

    free(mesh.vertices);
    free(mesh.indices);
}

void Draw(const Mesh& mesh)
{
    glBindVertexArray(mesh.vertexArray);

    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT, (void*)0);
}

