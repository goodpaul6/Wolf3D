#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdio.h>

#include "game.hpp"
#include "input.hpp"
#include "utils.hpp"

static const float LEVEL_SCALE_FACTOR = 2;

static void Forward(float lookAngle, float& x, float& z, float scale = 1)
{
    x = sinf(lookAngle) * scale;
    z = cosf(lookAngle) * scale;
}

static bool Collide(const Entity& e, float x, float z, const Level& level)
{
    if(!e.hasbb) return false;
    
    float minx = x + e.min.x;
    float maxx = x + e.max.x;
    
    float minz = z + e.min.z;
    float maxz = z + e.max.z;

    int left = (int)(minx / LEVEL_SCALE_FACTOR);
    int top = (int)(minz / LEVEL_SCALE_FACTOR);
    int right = (int)(maxx / LEVEL_SCALE_FACTOR);
    int bottom = (int)(maxz / LEVEL_SCALE_FACTOR);

    for(int zz = top; zz <= bottom; ++zz)
    {
        if(zz < 0) continue;
        if(zz >= level.height) break;

        for(int xx = left; xx <= right; ++xx)
        {
            if(xx < 0) continue;
            if(xx >= level.width) break;

            if(level.tiles[xx + zz * level.width] > 0)
                return true;
        }
    }

    return false;
}

static bool Collide(const Entity& a, float x, float y, float z, const Entity& b)
{
    if(!a.hasbb || !b.hasbb) return false;

    glm::vec3 amin = glm::vec3(x, y, z) + a.min;
    glm::vec3 amax = glm::vec3(x, y, z) + a.max;

    glm::vec3 bmin = glm::vec3(b.x, b.y, b.z) + b.min;
    glm::vec3 bmax = glm::vec3(b.x, b.y, b.z) + b.max;

	if (amax.x < bmin.x || bmax.x < amin.x) return false;
	if (amax.y < bmin.y || bmax.y < amin.y) return false;
	if (amax.z < bmin.z || bmax.z < amin.z) return false;

    return true;
}

static void MoveBy(Entity& e, float x, float z, const Level& level)
{
    if(!Collide(e, e.x + x, e.z, level))
        e.x += x;
    if(!Collide(e, e.x, e.z + z, level))
        e.z += z;
}

static void MoveBy(Entity& e, float x, float y, float z, const Game& game)
{
    if(Collide(e, e.x + x, e.z, game.level))
        x = 0;

    if(Collide(e, e.x, e.z + z, game.level))
        z = 0;

    // TODO: Clean this up so it's not doing float cmp
    if(x != 0)
    {
        for(int i = 0; i < game.doorCount; ++i)
        {
            if(Collide(e, e.x + x, e.y, e.z, game.doors[i]))
            {
                x = 0;
                break;
            }
        }

        e.x += x;
    }

    if(y != 0)
    {
        for(int i = 0; i < game.doorCount; ++i)
        {
            if(Collide(e, e.x, e.y + y, e.z, game.doors[i]))
            {
                y = 0;
                break;
            }
        }

        e.y += y;
    }
    
    if(z != 0)
    {
        for(int i = 0; i < game.doorCount; ++i)
        {
            if(Collide(e, e.x, e.y, e.z + z, game.doors[i]))
            {
                z = 0;
                break;
            }
        }

        e.z += z;
    }
}

static void Update(Player& player, const Game& game, float dt)
{
    if(!IsKeyDown(SDL_SCANCODE_LSHIFT))
    {
        if(IsKeyDown(SDL_SCANCODE_A))
            player.lookAngle += 2.5f * dt;
        else if(IsKeyDown(SDL_SCANCODE_D))
            player.lookAngle -= 2.5f * dt;
    }

    bool move = false;
    float moveAngle = 0;

    if(IsKeyDown(SDL_SCANCODE_LSHIFT))
    {
        if(IsKeyDown(SDL_SCANCODE_A))
        {
            moveAngle = player.lookAngle + (float)M_PI / 2.0f;
            move = true;
        }
        else if(IsKeyDown(SDL_SCANCODE_D))
        {
            moveAngle = player.lookAngle - (float)M_PI / 2.0f;
            move = true;
        }
    }

    if(IsKeyDown(SDL_SCANCODE_W))
    {
        moveAngle = player.lookAngle;

        if(IsKeyDown(SDL_SCANCODE_LSHIFT))
        {
            if(IsKeyDown(SDL_SCANCODE_A))
                moveAngle += (float)M_PI / 4.0f;
            else if(IsKeyDown(SDL_SCANCODE_D))
                moveAngle -= (float)M_PI / 4.0f;
        }

        move = true;
    }
    else if(IsKeyDown(SDL_SCANCODE_S))
    {
        moveAngle = player.lookAngle + (float)M_PI;

        if(IsKeyDown(SDL_SCANCODE_LSHIFT))
        {
            if(IsKeyDown(SDL_SCANCODE_A))
                moveAngle -= (float)M_PI / 4.0f;
            else if(IsKeyDown(SDL_SCANCODE_D))
                moveAngle += (float)M_PI / 4.0f;
        }

        move = true;
    }

    float x, z;
    Forward(moveAngle, x, z, 6 * dt);

    if(move)
    {
        MoveBy(player, x, 0, z, game);
        player.y = sinf(player.stride) / 20.0f;

        player.stride += 10 * dt;
    }

    if(IsKeyDown(SDL_SCANCODE_E))
    {
        // Open doors in front of us
        for(int i = 0; i < game.doorCount; ++i)
        {
            if(Collide(player, player.x + x, player.y, player.z + z, game.doors[i]))
            {
                if(!game.doors[i].open)
                {
                    game.doors[i].open = true;
                    game.doors[i].x += 1.5f;
                }
            }
        }
    }
}

void Init(Game& game)
{
    game.level = LoadLevel("levels/test.map");

    game.basicShader = LoadShader("shaders/basic.vert", "shaders/basic.frag");

    game.levelTexture = LoadTexture("textures/wolf.png");
    game.gunTexture = LoadTexture("textures/pistol.png");

    game.gunMesh = CreatePlaneMesh();

    float u1, v1, u2, v2; 
    GetTileUV(game.levelTexture, 28, u1, v1, u2, v2);

    game.doorMesh = CreatePlaneMesh(u1, v1, u2, v2);
    game.levelMesh = CreateLevelMesh(game.level, game.levelTexture);

    game.player.hasbb = true;
    game.player.min = glm::vec3(-0.5f, -1, -0.5f);
    game.player.max = glm::vec3(0.5f, 1, 0.5f);
    
    // Setup entities
    for(int i = 0; i < game.level.typeCount; ++i)
    {
        if(strcmp(game.level.types[i], "player") == 0)
        {
            if(game.level.entityCount[i] != 1)
                CRASH("Invalid number of players on the map.\n");
            
            const EntityInfo& info = game.level.entities[i][0];

            game.player.x = info.x;
            game.player.y = info.y;
            game.player.z = info.z;
        }
        else if(strcmp(game.level.types[i], "door") == 0)
        {
            game.doorCount = game.level.entityCount[i];
            game.doors = (Door*)calloc(game.doorCount, sizeof(Door));

            for(int j = 0; j < game.doorCount; ++j)
            {
                const EntityInfo& info = game.level.entities[i][j];
                Door& door = game.doors[j];

                door.x = info.x;
                door.y = info.y;
                door.z = info.z;

                door.hasbb = true;
                door.min = glm::vec3(-1, 0, -0.15f);
                door.max = glm::vec3(1, 1, 0.15f);
            }
        }
    }
}

void Update(Game& game, float dt)
{
    Update(game.player, game, dt);
}

void Draw(const Game& game, const glm::mat4& proj)
{
	glUseProgram(game.basicShader.id);
    
    GLuint projLoc = glGetUniformLocation(game.basicShader.id, "proj"); 
    GLuint texLoc = glGetUniformLocation(game.basicShader.id, "tex");
	GLuint modelLoc = glGetUniformLocation(game.basicShader.id, "model");
	GLuint viewLoc = glGetUniformLocation(game.basicShader.id, "view");

    // Setup view proj
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

    glm::mat4 view = glm::lookAt(glm::vec3(game.player.x, game.player.y, game.player.z), 
                                 glm::vec3(game.player.x + sinf(game.player.lookAngle), game.player.y, game.player.z + cosf(game.player.lookAngle)), 
                                 glm::vec3(0, 1, 0));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    
    // Draw level
    glm::mat4 model = glm::translate(glm::vec3(0, -1, 0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, game.levelTexture.id);
        
    glUniform1i(texLoc, 0);

    Draw(game.levelMesh);

    // Draw doors
    for(int i = 0; i < game.doorCount; ++i)
    {
        float x = 0;

        // TODO: Make actual door model instead of copying plane twice
        model = glm::translate(glm::vec3(game.doors[i].x + x, game.doors[i].y, game.doors[i].z));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        Draw(game.doorMesh);

        model = glm::translate(glm::vec3(game.doors[i].x + x, game.doors[i].y, game.doors[i].z + 0.3f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        Draw(game.doorMesh);
    }

    // Draw player gun
    float x, z;
    Forward(game.player.lookAngle, x, z, 1.2f);

    // Get the right vector (which is used to bob the gun across)
    float rx, rz;
    Forward(game.player.lookAngle - (float)M_PI / 2, rx, rz);

    float t = sinf(game.player.stride / 2.0f);

    x += rx * t * 0.02f;
    z += rz * t * 0.02f;
    float y = sinf(game.player.stride) * 0.01f - 0.01f;
    
    float tilt = sinf(game.player.stride) * 1.4f;

    model = glm::translate(glm::vec3(game.player.x + x, game.player.y + y, game.player.z + z)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)) * glm::rotate(game.player.lookAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindTexture(GL_TEXTURE_2D, game.gunTexture.id);

    glDisable(GL_DEPTH_TEST); 
    Draw(game.gunMesh);
    glEnable(GL_DEPTH_TEST);
}

void Destroy(Game& game)
{
    free(game.doors);

    DestroyLevel(game.level);

    DestroyMesh(game.gunMesh);
    DestroyMesh(game.levelMesh);
}
