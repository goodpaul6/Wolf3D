#pragma once

#include "graphics.hpp"
#include "resources.hpp"

struct Entity
{
    bool hasbb = false;
    glm::vec3 min, max;          // local positions (relative to entity)

    float x = 0, y = 0, z = 0;
};

struct Player : public Entity
{
    float lookAngle = 0;         // radians
    float stride = 0;

    int gunFrame = 1;
};

struct Door : public Entity
{
    int dir = 0;
    
    float sx = 0, sz = 0;
    bool open = false;
    float openness = 0.0f; 
};

struct Game
{
    Player player;

    int doorCount = 0;
    Door* doors = nullptr;
    
    Level level;

    mutable Mesh levelMesh;
    mutable Mesh gunMesh;
    mutable Mesh doorMesh;

    Texture levelTexture;
    Texture doorTexture;
    Texture gunTexture;

    Shader basicShader;
};

void Init(Game& game);
void Update(Game& game, float dt);
void Draw(const Game& game, const glm::mat4& proj);
void Destroy(Game& game);
