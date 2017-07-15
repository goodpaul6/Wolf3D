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

    bool shoot = false;         // shoot animation enabled
    float animTimer = 0;
    int frame = 0;
};

struct Door : public Entity
{
    int dir = 0;
    
    float sx = 0, sz = 0;
    bool open = false;
    float openness = 0.0f; 
};

struct Enemy : public Entity
{
    float lookAngle = 0;
    int health = 1;
    float speed = 2;
    float hitTimer = 0;

    int frame = 0;
};

struct Game
{
    Player player;

    int doorCount = 0;
    Door* doors = nullptr;

    int enemyCount = 0;
    Enemy* enemies = nullptr;
    
    Level level;

    mutable Mesh enemyMesh;
    mutable Mesh levelMesh;
    mutable Mesh gunMesh;
    mutable Mesh doorMesh;
    mutable Mesh boxMesh;

    Texture levelTexture;
    Texture doorTexture;
    Texture gunTexture;
    Texture enemyTexture;
    Texture whiteTexture;

    Shader basicShader;
};

void Init(Game& game);
void Update(Game& game, float dt);
void Draw(const Game& game, const glm::mat4& proj);
void Destroy(Game& game);
