#pragma once

#include <stdint.h>

#include "resources.hpp"
#include "graphics.hpp"

static const int GAME_MAX_BULLET_IMPACTS = 64;
static const int GAME_MAX_TRACERS = 32;

struct Entity
{
    bool hasbb = false;
    glm::vec3 min, max;          // local positions (relative to entity)

    float x = 0, y = 0, z = 0;
};

struct Player : public Entity
{
    float pitch = 0;             // radians
    float lookAngle = 0;         // radians
    float stride = 0;

    bool shoot = false;         // shoot animation enabled
    float animTimer = 0;
    int frame = 0, lastFrame = 0;
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
    enum State : uint8_t
    {
        IDLE,
        WALKING,
        SAW_PLAYER,
        START_CHASE,
        CHASING,
        SHOOTING,
        DEAD
    } state = IDLE;

    float lookAngle = 0;
    int health = 1;
    float speed = 2;
    float hitTimer = 0;
    float animTimer = 0;
    float stateTimer = 0;

    int frame = 0;
};

struct Painting : public Entity
{
    int dir = 0;
    float angle = 0;
    float angularVel = 0;

    // It falls once hit
    bool hit = false;
};

struct Tracer : public Entity
{
    float life = 0;
    float shotAngle = 0;
};

struct Impact : public Entity
{
    float life = 0;
    int dir = 0;
};

struct Game
{
    Player player;

    int doorCount = 0;
    Door* doors = nullptr;

    int enemyCount = 0;
    Enemy* enemies = nullptr;

    int paintingCount = 0;
    Painting* paintings = nullptr;

    // Just a bunch of bounding boxes
    int boxColliderCount = 0;
    Entity* boxColliders = nullptr;
    
    Impact impacts[GAME_MAX_BULLET_IMPACTS];
    Tracer tracers[GAME_MAX_TRACERS];
    
    Level level;

    bool debugDraw = false;

    mutable Mesh enemyMesh;
    mutable Mesh levelMesh;
    mutable Mesh gunMesh;
    mutable Mesh doorMesh;
    mutable Mesh paintingMesh;
    mutable Mesh boxMesh;
    mutable Mesh planeMesh;

    mutable Quad quad;

    Texture levelTexture;
    Texture doorTexture;
    Texture gunTexture;
    Texture enemyTexture;
    Texture whiteTexture;
    Texture paintingTexture;
    Texture paintingHitTexture;
    Texture bulletImpactTexture;
    Texture tracerTexture;

    Shader basicShader;
    Shader spriteShader;
};

void Init(Game& game);
void Update(Game& game, float dt);
void Draw(const Game& game, const glm::mat4& proj);
void Destroy(Game& game);
