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
    Vertex* vertices = (Vertex*)malloc(sizeof(Vertex) * 4 * level.planeCount); 
    
    int indexCount = 0;
	ushort* indices = (ushort*)malloc(sizeof(ushort) * 6 * level.planeCount);

    for(int i = 0; i < level.planeCount; ++i)
    {
        const Level::Plane& plane = level.planes[i];

        int oa[3] = { plane.a[0] - plane.o[0], plane.a[1] - plane.o[1], plane.a[2] - plane.o[2] };
        int c[3] = { plane.b[0] - oa[0], plane.b[1] - oa[1], plane.b[2] - oa[2] };

        const float size = LEVEL_SCALE_FACTOR / 2.0f;
    
        float x1 = plane.o[0] * size;
        float y1 = plane.o[1] * size;
        float z1 = plane.o[2] * size;

        float x2 = plane.a[0] * size;
        float y2 = plane.a[1] * size;
        float z2 = plane.a[2] * size;
        
        float x3 = plane.b[0] * size;
        float y3 = plane.b[1] * size;
        float z3 = plane.b[2] * size;

        float x4 = c[0] * size;
        float y4 = c[1] * size;
        float z4 = c[2] * size;
        
        float u1, v1, u2, v2;
        GetTileUV(texture, plane.tile, u1, v1, u2, v2);

        const Vertex verts[] =
        {
            { x1, y1, z1, u1, v1 },
            { x2, y2, z2, u2, v1 },
            { x3, y3, z3, u2, v2 },
            { x4, y4, z4, u1, v2 }
        };

		ushort idx = (ushort)vertexCount;

        memcpy((void*)&vertices[vertexCount], verts, sizeof(verts));
        vertexCount += COUNT_OF(verts);

        const ushort ind[] =
        {
            idx, idx + 1, idx + 2,
            idx + 2, idx + 3, idx
        };

        memcpy((void*)&indices[indexCount], ind, sizeof(ind));
        indexCount += COUNT_OF(ind);
    }
    
    Mesh mesh = CreateMesh(vertexCount, vertices, indexCount, indices);
    
    free(vertices);
    free(indices);

    return mesh;
}

void PlaneShowFrame(Mesh& mesh, const Texture& texture, int fw, int fh, int frame)
{
    int columns = texture.width / fw;

    float w = (float)fw / texture.width;
    float h = (float)fh / texture.height;

    float u = (frame % columns) * w;
    float v = (frame / columns) * h;

    const Vertex vertices[] =
    {
        // Top-left
        { -1, 1, 0, u, v },
        // Top-right
        { 1, 1, 0, u + w, v },
        // Bottom-right
        { 1, -1, 0, u + w, v + h },
        // Bottom-left
        { -1, -1, 0, u, v + h }
    };
    
    // TODO: Check if this is safe to do without binding vertex array

    glBindBuffer(GL_ARRAY_BUFFER, mesh.buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
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

