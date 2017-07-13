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
};

struct Door : public Entity
{
    bool open = false;
};

struct Game
{
    Player player;

    int doorCount = 0;
    Door* doors = nullptr;
    
    Level level;

    Mesh levelMesh;
    Mesh gunMesh;
    Mesh doorMesh;

    Texture levelTexture;
    Texture gunTexture;

    Shader basicShader;
};

void Init(Game& game);
void Update(Game& game, float dt);
void Draw(const Game& game, const glm::mat4& proj);
void Destroy(Game& game);
