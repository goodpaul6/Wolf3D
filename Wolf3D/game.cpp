#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdio.h>

#include "game.hpp"
#include "input.hpp"
#include "utils.hpp"

// NOTES:
// - Determining impact location can be generalized by clipping 
//   the incident ray to the bounding box of the object that was
//   hit (just like it is done in RayLevel)

// TODO:
// - RayLevel should only be called for precise hit location information
//   and this should only be required at the end of Shoot() where bullet
//   impacts are created. Otherwise, we just get the ray bounds and use
//   LevelHasTile()

static const int RAY_BOX_HIT_SAMPLE_COUNT = 20;
static const int MAX_SHOT_LEVEL_INTERSECTIONS = 20;
static const float PLAYER_DOOR_OPEN_DIST = 2.5f;
static const float DOOR_OPEN_SPEED = 1.5f;
static const float DOOR_OPEN_AMOUNT = 1.5f;
static const float ENEMY_HIT_TIME = 1.5f;
static const float IMPACT_LIFE = 1.5f;
static const float TRACER_MOVE_SPEED = 60.0f;
static const float TRACER_Y_OFF = -0.2f;
static const float TRACER_LIFE = 1.0f;

static bool LinePlane(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec3& planeOrigin, const glm::vec3& planeNormal, glm::vec3* hit = nullptr)
{
    float startDistToPlane = glm::dot((lineStart - planeOrigin), planeNormal);
    float projLength = glm::dot((lineStart - lineEnd), planeNormal);
    float scale = startDistToPlane / projLength;

    // The starting distance is negative (behind the plane) and 
    // the endpoint is also behind the plane
    if(scale < 0)
        return false;

    // The distance from the start of the line to the plane is longer than
    // the length of the line
    if(scale > 1.0f)
        return false;

    return true;
}

static glm::vec3 Center(const Entity& a)
{
    if(!a.hasbb) return glm::vec3(a.x, a.y, a.z);
    return (glm::vec3(a.x, a.y, a.z) * 2.0f + a.min + a.max) / 2.0f;
}

static float Dist2(const Entity& a, const Entity& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;

    return dx * dx + dy * dy + dz * dz;
}

static void Forward(float lookAngle, float& x, float& z, float scale = 1)
{
    x = sinf(lookAngle) * scale;
    z = cosf(lookAngle) * scale;
}

// Projects the x, y coords onto each direction's vector,
// picking the one with the largest projection
static int VecToDir(float x, float z)
{
    const float dirs[4][2] =
    {
        // x, z
        { 0, 1 },
        { 1, 0 },
        { 0, -1 },
        { -1, 0 }
    };

    int maxDir = 0;
    float maxDot = 0;

    for(int i = 0; i < 4; ++i) 
    {
        float dot = dirs[i][0] * x + dirs[i][1] * z;
        
        if(dot > maxDot)
        {
            maxDir = i;
            maxDot = dot;
        }
    }
    
    return maxDir;
}

static void CreateImpact(Game& game, float x, float y, float z, int dir)
{
    for(int i = 0; i < GAME_MAX_BULLET_IMPACTS; ++i)
    {
        if(game.impacts[i].life <= 0)
        {
            game.impacts[i].x = x;
            game.impacts[i].y = y;
            game.impacts[i].z = z;
            game.impacts[i].life = IMPACT_LIFE;
            game.impacts[i].dir = dir;

            return;
        }
    }
}

static void CreateTracer(Game& game, float x, float y, float z, float angle)
{
    for(int i = 0; i < GAME_MAX_BULLET_IMPACTS; ++i)
    {
        if(game.tracers[i].life <= 0)
        {
            game.tracers[i].x = x;
            game.tracers[i].y = y;
            game.tracers[i].z = z;
            game.tracers[i].life = TRACER_LIFE;
            game.tracers[i].shotAngle = angle;

            return;
        }
    }
}

static bool PointInBox(const glm::vec3& p, const glm::vec3& pos, const glm::vec3& min, const glm::vec3& max)
{
    glm::vec3 pmin = pos + min;
    glm::vec3 pmax = pos + max;

    return p.x > pmin.x && p.x < pmax.x &&
           p.y > pmin.y && p.y < pmax.y &&
           p.z > pmin.z && p.z < pmax.z;
}

// Assumes direction is normalized
static bool RayBox(const glm::vec3& start, const glm::vec3& dir, const glm::vec3& pos, const glm::vec3& min, const glm::vec3& max, glm::vec3* hit = nullptr)
{
#if 0
    float lowProj = -FLT_MAX;
    float highProj = FLT_MAX;

    for(int i = 0; i < 3; ++i)
    {
        float minProj = ((pos[i] + min[i]) - start[i]) / dir[i];
        float maxProj = ((pos[i] + max[i]) - start[i]) / dir[i];

        if(minProj > maxProj)
        {
            float tmp = minProj;
            maxProj = minProj;
            minProj = tmp;
        }

        if(maxProj < lowProj || minProj > highProj)
            return false;

        if(minProj > lowProj) lowProj = minProj;
        if(maxProj < highProj) highProj = maxProj;
    }

    bool result = lowProj > highProj ? false : true;

    if(result && hit)
    {
        *hit = start + dir * lowProj;
        return true;
    }

    return result;
#else 
    glm::vec3 c = (min + max) / 2.0f + pos;

    glm::vec3 diff = c - start;
    float dot = glm::dot(diff, dir);
    
    if(dot < 0) return false;

    glm::vec3 p = start + dir * dot;

    glm::vec3 pdiff = c - p;

    glm::vec3 clampedDiff = glm::clamp(pdiff, min, max);

    if(glm::length2(clampedDiff) >= glm::length2(pdiff))
    {
        if(hit)
        {
            // TODO: Implement properly

            // Linearly step from the the start and the perpendicular length point p
            // until you hit the wall, move back and call that the hit location
            const float tmove = dot / RAY_BOX_HIT_SAMPLE_COUNT;
            float t = 0;

            glm::vec3 pt;
            for(int i = 0; i < RAY_BOX_HIT_SAMPLE_COUNT; ++i)
			{
				pt = start + dir * t;
                t += tmove;

				if (PointInBox(pt, pos, min, max))
					break;
            };

            *hit = pt;
        }

        return true;
    }

    return false;
#endif
}

// TODO: Get rid of level tile stuff
static bool LevelHasTile(const Level& level, int left, int top, int right, int bottom)
{
    return false;
#if 0
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
#endif
}

// origin = origin of the ray, delta = offset from the origin to the target
static bool RayLevel(const glm::vec2& origin, const glm::vec2& delta, const Level& level, glm::vec2& hit, int& dir)
{
    return false;
// TODO: Check against plane based geometry
#if 0
    // Advance from origin sample count times and check if a wall was hit or
    // we go out of bounds
    for(int i = 0; i < LEVEL_RAY_SAMPLE_COUNT; ++i)
    {
        glm::vec2 pos = origin + (delta / (float)LEVEL_RAY_SAMPLE_COUNT) * (float)i;

        int tx = (int)(pos.x / LEVEL_SCALE_FACTOR);
        int ty = (int)(pos.y / LEVEL_SCALE_FACTOR);
    
        if(tx < 0 || tx >= level.width) continue;
        if(ty < 0 || ty >= level.height) continue;

        if(level.tiles[tx + ty * level.width] > 0)
        {
            // Center of tile (in world space)
            glm::vec2 c(tx * LEVEL_SCALE_FACTOR + LEVEL_SCALE_FACTOR / 2, 
                        ty * LEVEL_SCALE_FACTOR + LEVEL_SCALE_FACTOR / 2);

            // Get closest edge

            // Vector from center of tile to origin (clamped to extents of tile)
            /*glm::vec2 a = glm::clamp(origin - c, glm::vec2(-LEVEL_SCALE_FACTOR / 2,
                                                           -LEVEL_SCALE_FACTOR / 2),
                                                 glm::vec2(LEVEL_SCALE_FACTOR / 2,
                                                           LEVEL_SCALE_FACTOR / 2));

            // Added back to tile center 
            hit = c + a;*/
        
            hit = pos - delta / (float)LEVEL_RAY_SAMPLE_COUNT;
            dir = VecToDir((hit - c).x, (hit - c).y);

	        return true;
        }
    }
    
    return false;
#endif
}

static bool CheckShot(float x, float y, float z, float angle, const Entity& target, Game& game)
{ 
    glm::vec3 start{x, y, z};
    glm::vec3 dir{sinf(angle), 0, cosf(angle)}; 
    
    // TODO: Think of a more optimal solution than checking every door
    // Check to see if there are any doors in the way
    for(int i = 0; i < game.doorCount; ++i)
    {
        const Door& door = game.doors[i];
        
        // Check to see if the door is closer to us than the target
        // If it s farther, we can assume the door isn't in the way
        if(glm::length2(glm::vec3(door.x, door.y, door.z) - start) >
           glm::length2(glm::vec3(target.x, target.y, target.z) - start))
            continue;

        // Check for ray collision with the door
        if(RayBox(start, dir, glm::vec3(door.x, door.y, door.z), door.min, door.max))
            return false;
    }

    for(int i = 0; i < game.boxColliderCount; ++i)
    {
        const Entity& box = game.boxColliders[i];

        if(RayBox(glm::vec3(x, y, z), glm::vec3(sinf(angle), 0, cosf(angle)), glm::vec3(box.x, box.y, box.z), box.min, box.max))
            return false;
    }

	if (!target.hasbb) return false;

    return RayBox(start, dir, glm::vec3(target.x, target.y, target.z), target.min, target.max);
}

static void Shoot(float x, float y, float z, float angle, Game& game)
{
    // TODO: Think about whether tracers are even necessary
    //CreateTracer(game, x, y + TRACER_Y_OFF, z, angle);

    for(int i = 0; i < game.enemyCount; ++i)
    {
        Enemy& enemy = game.enemies[i];

        if(enemy.health <= 0)
            continue;     

        if(CheckShot(x, y, z, angle, enemy, game))
        {
            enemy.health -= 1;

            if(enemy.health <= 0)
                enemy.frame = 1;
            else
                enemy.hitTimer = ENEMY_HIT_TIME;

			return;
        }
    }

    for(int i = 0; i < game.paintingCount; ++i)
    {
        Painting& painting = game.paintings[i];

        if(CheckShot(x, y, z, angle, painting, game))
        {
            if(!painting.hit)
                painting.hit = true;
            painting.angularVel += ((rand() / (float)RAND_MAX) - 0.5f);
            return;
        }
    }

    // TODO: Check if doors and stuff are in the way and only then generate impacts

    // Hit locations
    int hitCount = 0;
    static int boxIndices[MAX_SHOT_LEVEL_INTERSECTIONS];
    static glm::vec3 hits[MAX_SHOT_LEVEL_INTERSECTIONS]; 
    
    for(int i = 0; i < game.boxColliderCount; ++i)
    {
        if(hitCount >= MAX_SHOT_LEVEL_INTERSECTIONS)
        {
            //printf("Discarding %d hit checks.\n", game.boxColliderCount - (i + 1));
            break;
        }

        const Entity& box = game.boxColliders[i];

        if(RayBox(glm::vec3(x, y, z), glm::vec3(sinf(angle), 0, cosf(angle)), glm::vec3(box.x, box.y, box.z), box.min, box.max, &hits[hitCount]))
        {
            boxIndices[hitCount] = i;
            hitCount += 1;
        }
    }

    if(hitCount > 0)
    {
        int closestHit = 0;
        float minHitDist2 = FLT_MAX;

        for(int i = 0; i < hitCount; ++i)
        {
            float d2 = glm::length2(hits[i] - glm::vec3(x, y, z)); 
            if(d2 < minHitDist2)
            {
                minHitDist2 = d2;
                closestHit = i;
            }
        }

        glm::vec3 hit = hits[closestHit];
        glm::vec3 boxCenter = Center(game.boxColliders[boxIndices[closestHit]]);

        int dir = VecToDir(hit.x - boxCenter.x, hit.z - boxCenter.z);
        CreateImpact(game, hit.x, hit.y, hit.z, dir);
    }
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

    return LevelHasTile(level, left, top, right, bottom);
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

static bool CollideSolids(const Entity& e, float x, float y, float z, const Game& game)
{
    for(int i = 0; i < game.boxColliderCount; ++i)
    {
        if(Collide(e, x, y, z, game.boxColliders[i]))
            return true;
    }

    for(int i = 0; i < game.doorCount; ++i)
    {
        if(Collide(e, x, y, z, game.doors[i]))
            return true;
    }

    for(int i = 0; i < game.paintingCount; ++i)
    {
        if(Collide(e, x, y, z, game.paintings[i]))
            return true;
    }

    return false;
}

static void MoveBy(Entity& e, float x, float y, float z, const Game& game)
{
    // TODO: Clean this up so it's not doing float cmp
    if(x != 0)
    {
        if(CollideSolids(e, e.x + x, e.y, e.z, game)) x = 0;
        e.x += x;
    }

    if(y != 0)
    {
        if(CollideSolids(e, e.x, e.y + y, e.z, game)) y = 0;
        e.y += y;
    }
    
    if(z != 0)
    {
        if(CollideSolids(e, e.x, e.y, e.z + z, game)) z = 0;
        e.z += z;
    }
}

static void Update(Player& player, Game& game, float dt)
{
    if(!IsKeyDown(SDL_SCANCODE_LSHIFT))
    {	
        if(IsKeyDown(SDL_SCANCODE_A))
            player.lookAngle += 2.5f * dt;
        else if(IsKeyDown(SDL_SCANCODE_D))
            player.lookAngle -= 2.5f * dt;
    }

    if(player.shoot)
    {
        if(player.animTimer < 0.1f) 
        {
            player.animTimer += dt;
            player.frame = (int)(player.animTimer / 0.02f);
            
            if(player.lastFrame != 3 && player.frame == 3)
                Shoot(player.x, 0, player.z, player.lookAngle + ((float)rand() / RAND_MAX) * 0.02f - 0.01f, game);

            player.lastFrame = player.frame;
        }
        else
        {
            player.animTimer = 0;
            player.shoot = false;
        }
    }
    
    if(IsKeyDown(SDL_SCANCODE_SPACE) && !player.shoot)
    {
        // FIXME: We don't shoot from player.y because
        // that bobs below the bounding boxes of enemies
        // Maybe we shouldn't move the player y pos directly
        // and just render with the bob
        //Shoot(player.x, 0, player.z, player.lookAngle, game);
        player.shoot = true; 
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

    if(WasKeyPressed(SDL_SCANCODE_E))
    {
        // Open doors near us
        for(int i = 0; i < game.doorCount; ++i)
        {
			if (Dist2(game.doors[i], player) < PLAYER_DOOR_OPEN_DIST * PLAYER_DOOR_OPEN_DIST)
				game.doors[i].open = !game.doors[i].open;
        }
    }

    if(WasKeyPressed(SDL_SCANCODE_P))
        printf("Player pos: %f %f %f\n", player.x, player.y, player.z);
}

static void Update(Door& door, float dt)
{
    if(door.open)
    {
        if(door.openness < 1)
            door.openness += DOOR_OPEN_SPEED * dt; 
    }
    else
    {
        if(door.openness > 0)
            door.openness -= DOOR_OPEN_SPEED * dt;
    }

    float xmov = door.openness * sinf(glm::radians(90.0f * door.dir)) * DOOR_OPEN_AMOUNT;
    float zmov = door.openness * cosf(glm::radians(90.0f * door.dir)) * DOOR_OPEN_AMOUNT;

    door.x = door.sx + xmov;
    door.z = door.sz + zmov;
}

static void Update(Enemy& enemy, float dt, Game& game)
{
    enemy.lookAngle = game.player.lookAngle + (float)M_PI;
    
    if(enemy.health > 0)
    {
        if(enemy.hitTimer > 0)
        {
            enemy.hitTimer -= dt;
            enemy.lookAngle += sinf(enemy.hitTimer) * 1.2f;
            return;
        }

        float dx = game.player.x - enemy.x;
        float dz = game.player.z - enemy.z;

        float moveAngle = atan2f(dx, dz);

        float mx, mz;
        Forward(moveAngle, mx, mz, dt * enemy.speed);

        MoveBy(enemy, mx, 0, mz, game);
    }
}

static void Update(Painting& painting, float dt)
{
    if(painting.hit)
    {
        painting.angle += painting.angularVel * dt;
        painting.angularVel += -painting.angle * 20 * dt;
    }
}

void Init(Game& game)
{
    game.level = LoadLevel("levels/test.map");

    game.basicShader = LoadShader("shaders/basic.vert", "shaders/basic.frag");

    game.levelTexture = LoadTexture("textures/wolf.png");
    
    // FIXME: This is technically redundant 
    glBindTexture(GL_TEXTURE_2D, game.levelTexture.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    game.doorTexture = LoadTexture("textures/door.png");
    game.gunTexture = LoadTexture("textures/pistol.png");
    game.enemyTexture = LoadTexture("textures/guard.png");
    game.whiteTexture = LoadTexture("textures/white.png");
    game.paintingTexture = LoadTexture("textures/painting1.png");
    game.paintingHitTexture = LoadTexture("textures/painting1_hit.png");
    game.bulletImpactTexture = LoadTexture("textures/bulletimpact.png");
    game.tracerTexture = LoadTexture("textures/tracer.png");

    game.gunMesh = CreatePlaneMesh();
    game.enemyMesh = CreatePlaneMesh();
    game.paintingMesh = LoadMesh("models/painting.obj");
    game.doorMesh = LoadMesh("models/door.obj");
    game.boxMesh = LoadMesh("models/box.obj");
    game.planeMesh = CreatePlaneMesh();
    game.levelMesh = CreateLevelMesh(game.level, game.levelTexture);

    game.player.hasbb = true;
    game.player.min = glm::vec3(-0.5f, -1, -0.5f);
    game.player.max = glm::vec3(0.5f, 1, 0.5f);
    
    // Setup entities
    
    // Make sure one player is on level
    if(game.level.entityCount[ET_PLAYER] != 1)
        CRASH("Invalid number of players on level\n");

    const EntityInfo& playerInfo = game.level.entities[ET_PLAYER][0];

    game.player.x = playerInfo.x;
    game.player.y = playerInfo.y;
    game.player.z = playerInfo.z;

    game.doorCount = game.level.entityCount[ET_DOOR];
    game.doors = new Door[game.doorCount];

    for(int i = 0; i < game.doorCount; ++i)
    {
        const EntityInfo& info = game.level.entities[ET_DOOR][i];
        Door& door = game.doors[i];

        door.dir = info.dir;
        door.x = door.sx = info.x;
        door.y = info.y;
        door.z = door.sz = info.z;

        door.hasbb = true;

        if(door.dir % 2 == 1)
        {
            door.min = glm::vec3(-1, -0.5f, -0.2f);
            door.max = glm::vec3(1, 0.5f, 0.2f);
        }
        else
        {
            door.min = glm::vec3(-0.2f, -0.5f, -1);
            door.max = glm::vec3(0.2f, 0.5f, 1);
        }
    }

    game.enemyCount = game.level.entityCount[ET_ENEMY];
    game.enemies = new Enemy[game.enemyCount];

    for(int i = 0; i < game.enemyCount; ++i)
    {
        const EntityInfo& info = game.level.entities[ET_ENEMY][i];
        Enemy& enemy = game.enemies[i];

        enemy.x = info.x;
        enemy.y = info.y;
        enemy.z = info.z;

        enemy.hasbb = true;
        enemy.min = glm::vec3(-0.3f, -0.5f, -0.3f);
        enemy.max = glm::vec3(0.3f, 0.5f, 0.3f);

        enemy.health = info.health;
        enemy.speed = info.speed;
    }

    game.paintingCount = game.level.entityCount[ET_PAINTING];
    game.paintings = new Painting[game.paintingCount];

    for(int i = 0; i < game.paintingCount; ++i)
    {
        const EntityInfo& info = game.level.entities[ET_PAINTING][i];
        Painting& painting = game.paintings[i];

        painting.x = info.x;
        painting.y = info.y;
        painting.z = info.z;

        painting.dir = info.dir;

        painting.hasbb = true;
        if(painting.dir % 2 == 1)
        {
            painting.min = glm::vec3(-0.3f, -0.3f, -1);
            painting.max = glm::vec3(0.3f, 0.3f, 1);
        }
        else
        {
            painting.min = glm::vec3(-1, -0.3f, -0.3f);
            painting.max = glm::vec3(1, 0.3f, 0.3f);
        }
    }

    game.boxColliderCount = game.level.entityCount[ET_BOXCOLLIDER];
    game.boxColliders = new Entity[game.boxColliderCount];

    for(int i = 0; i < game.boxColliderCount; ++i)
    {
        const EntityInfo& info = game.level.entities[ET_BOXCOLLIDER][i];
        Entity& box = game.boxColliders[i];

        box.x = info.x;
        box.y = info.y;
        box.z = info.z;

        box.hasbb = true;
        box.min = glm::vec3(info.minx, info.miny, info.minz);
        box.max = glm::vec3(info.maxx, info.maxy, info.maxz);
    }
}

void Update(Game& game, float dt)
{
    Update(game.player, game, dt);

    for(int i = 0; i < game.doorCount; ++i)
        Update(game.doors[i], dt);
    
    for(int i = 0; i < game.enemyCount; ++i)
        Update(game.enemies[i], dt, game);

    for(int i = 0; i < game.paintingCount; ++i)
        Update(game.paintings[i], dt);

    for(int i = 0; i < GAME_MAX_BULLET_IMPACTS; ++i)
        game.impacts[i].life -= dt;

    for(int i = 0; i < GAME_MAX_TRACERS; ++i)
    {
        if(game.tracers[i].life <= 0) continue;

        float mx = sinf(game.tracers[i].shotAngle) * TRACER_MOVE_SPEED * dt;
        float mz = cosf(game.tracers[i].shotAngle) * TRACER_MOVE_SPEED * dt;

        game.tracers[i].x += mx;
        game.tracers[i].z += mz;

        game.tracers[i].life -= dt;
    }
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
    glBindTexture(GL_TEXTURE_2D, game.doorTexture.id);

    for(int i = 0; i < game.doorCount; ++i)
    {
        glm::mat4 rot = glm::rotate(glm::radians(90.0f * game.doors[i].dir), glm::vec3(0.0f, 1.0f, 0.0f));

        model = glm::translate(glm::vec3(game.doors[i].x, game.doors[i].y, game.doors[i].z)) * rot;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        Draw(game.doorMesh);
    }

    // Draw enemies
    glBindTexture(GL_TEXTURE_2D, game.enemyTexture.id);

	for (int i = 0; i < game.enemyCount; ++i)
	{
        glm::mat4 rot = glm::rotate(game.enemies[i].lookAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        // TODO: Remove this weird constant offset when the enemy dies
        float y = game.enemies[i].health > 0 ? 0 : -0.1f;

        glm::mat4 model = glm::translate(glm::vec3(game.enemies[i].x, game.enemies[i].y + y, game.enemies[i].z)) * rot;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        PlaneShowFrame(game.enemyMesh, game.enemyTexture, 64, 64, game.enemies[i].frame);
        Draw(game.enemyMesh);
	}
    
    // Draw paintings
    for(int i = 0; i < game.paintingCount; ++i)
    {
        float rads = glm::radians(game.paintings[i].dir * 90.0f);
    
        glm::mat4 rot = glm::rotate(rads, glm::vec3(0, 1, 0));
    
        glm::vec3 axis(sinf(rads), 0, cosf(rads));

        glm::mat4 shake = glm::rotate(game.paintings[i].angle, axis);

        glm::mat4 model = glm::translate(glm::vec3(game.paintings[i].x, game.paintings[i].y, game.paintings[i].z)) * rot * glm::translate(glm::vec3(0, 0.5f, 0)) * shake * glm::translate(glm::vec3(0, -0.5f, 0));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
        if(!game.paintings[i].hit)
            glBindTexture(GL_TEXTURE_2D, game.paintingTexture.id);
        else
            glBindTexture(GL_TEXTURE_2D, game.paintingHitTexture.id);

        Draw(game.paintingMesh);
    }

    // Draw bullet impacts 
    glBindTexture(GL_TEXTURE_2D, game.bulletImpactTexture.id);

    for(int i = 0; i < GAME_MAX_BULLET_IMPACTS; ++i)
    {
        if(game.impacts[i].life <= 0) continue;

        glm::mat4 rot = glm::rotate(glm::radians(game.impacts[i].dir * 90.0f), glm::vec3(0, 1, 0)) * glm::translate(glm::vec3(-0.070f, -0.070f, 0));

        glm::mat4 model = glm::translate(glm::vec3(game.impacts[i].x, game.impacts[i].y, game.impacts[i].z)) * rot;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        Draw(game.planeMesh);
    }

    // Draw tracers
    glBindTexture(GL_TEXTURE_2D, game.tracerTexture.id);
    
    for(int i = 0; i < GAME_MAX_TRACERS; ++i)
    {
        if(game.tracers[i].life <= 0) continue;

        glm::mat4 rot = glm::rotate(game.tracers[i].shotAngle, glm::vec3(0, 1, 0)) * glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0)) * glm::translate(glm::vec3(-0.01f, 0, 0));

        glm::mat4 trans = glm::translate(glm::vec3(game.tracers[i].x, game.tracers[i].y, game.tracers[i].z));

        glm::mat4 model = trans * rot;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        Draw(game.planeMesh);
        
        // Draw it twice (once again rotated 90 degrees in local z)
        rot = glm::rotate(game.tracers[i].shotAngle, glm::vec3(0, 1, 0)) * glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1)) * glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0)) * glm::translate(glm::vec3(-0.01f, 0, 0));

        model = trans * rot;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        Draw(game.planeMesh);
    }

#define DEBUG_DRAW
#ifdef DEBUG_DRAW
    // Draw box colliders
    glBindTexture(GL_TEXTURE_2D, game.whiteTexture.id);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	for (int i = 0; i < game.boxColliderCount; ++i)
	{
        glm::vec3 min = game.boxColliders[i].min;
        glm::vec3 max = game.boxColliders[i].max;

        glm::mat4 scale = glm::scale(glm::vec3(max.x - min.x,
                                               max.y - min.y,
                                               max.z - min.z));
        
        glm::mat4 model = glm::translate(glm::vec3(game.boxColliders[i].x, game.boxColliders[i].y, game.boxColliders[i].z)) * scale;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        Draw(game.boxMesh);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    // Draw player gun
    float x = 0, y = 0, z = 0;
    Forward(game.player.lookAngle, x, z, 1.2f);

    // Get the right vector (which is used to bob the gun across)
    float rx, rz;
    Forward(game.player.lookAngle - (float)M_PI / 2, rx, rz);

    float t = sinf(game.player.stride / 2.0f);

    // If the player isn't shooting (cause the gun must be positioned correctly)
    if(!game.player.shoot)
    {
        x += rx * t * 0.02f;
        z += rz * t * 0.02f;
        y = sinf(game.player.stride) * 0.01f - 0.01f;
    }

    model = glm::translate(glm::vec3(game.player.x + x, game.player.y + y, game.player.z + z)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)) * glm::rotate(game.player.lookAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindTexture(GL_TEXTURE_2D, game.gunTexture.id);

    PlaneShowFrame(game.gunMesh, game.gunTexture, 64, 64, game.player.frame);

    glDisable(GL_DEPTH_TEST); 
    Draw(game.gunMesh);
    glEnable(GL_DEPTH_TEST);
}

void Destroy(Game& game)
{
    delete game.doors;
    delete game.enemies;
    delete game.paintings;
    delete game.boxColliders;

    DestroyLevel(game.level);

    DestroyMesh(game.gunMesh);
    DestroyMesh(game.levelMesh);
}
