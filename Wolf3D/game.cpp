#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdio.h>

#include "game.hpp"
#include "input.hpp"
#include "utils.hpp"

static const int GAME_MAX_HIT_CHECK_ENTS = 20;
static const int MAX_SHOT_LEVEL_INTERSECTIONS = 20;
static const float PLAYER_DOOR_OPEN_DIST = 2.5f;
static const float DOOR_OPEN_SPEED = 1.5f;
static const float DOOR_OPEN_AMOUNT = 1.75f;
static const float ENEMY_HIT_TIME = 0.15f;
static const float ENEMY_IDLE_TIME = 2.0f;
static const float ENEMY_WALK_TIME = 2.0f;
static const float ENEMY_SHOOT_DIST = 5.0f;
static const float ENEMY_LOSE_SIGHT_DIST = 10.0f;
static const float ENEMY_SIGHT_DIST = 7.0f;
static const float ENEMY_SAW_PLAYER_TIME = 0.25f;
static const float ENEMY_START_CHASE_TIME = 0.25f;
static const float IMPACT_LIFE = 10.0f;
static const float IMPACT_HOVER_EPSILON = 0.1f;
static const float TRACER_MOVE_SPEED = 1.0f;
static const float TRACER_Y_OFF = -0.2f;
static const float TRACER_LIFE = 10.0f;
static const float DIR_DEGREES = 45.0f;

inline static glm::vec3 Pos(const Entity& e)
{
    return glm::vec3(e.x, e.y, e.z);
}

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
// If major is true, then only the major directions are
// returned
static int VecToDir(float x, float z, bool major = false)
{
    const float dirs[8][2] =
    {
        // x, z
        { sinf(0.0f), cosf(0.0f) }, 
        { sinf((float)M_PI / 4), cosf((float)M_PI / 4) }, 
        { sinf((float)M_PI / 2), cosf((float)M_PI / 2) }, 
        { sinf((float)M_PI / 2 + (float)M_PI / 4), cosf((float)M_PI / 2 + (float)M_PI / 4) }, 
        { sinf((float)M_PI), cosf((float)M_PI) }, 
        { sinf((float)M_PI + (float)M_PI / 4), cosf((float)M_PI + (float)M_PI / 4) }, 
        { sinf((float)M_PI + (float)M_PI / 2), cosf((float)M_PI + (float)M_PI / 2) },
        { sinf((float)M_PI + (float)M_PI / 2 + (float)M_PI / 4), cosf((float)M_PI + (float)M_PI / 2 + (float)M_PI / 4) }
    };

    int maxDir = 0;
    float maxDot = 0;

    for(int i = 0; i < 8; ++i) 
    {
        if(major && i % 2 != 0) continue;

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
static bool RayBox(const glm::vec3& start, const glm::vec3& dir, const glm::vec3& pos, const glm::vec3& min, const glm::vec3& max, float* t = nullptr)
{
#if 1
	float tmin = -INFINITY;
	float tmax = INFINITY;

    for(int i = 0; i < 3; ++i)
    {
		if (dir[i] == 0.0f) continue;

        float tx1 = ((min[i] + pos[i]) - start[i])/dir[i];
        float tx2 = ((max[i] + pos[i]) - start[i])/dir[i];

        tmin = max(tmin, min(tx1, tx2));
        tmax = min(tmax, max(tx1, tx2));
    }

    *t = tmin;

    return tmax >= 0 && tmax >= tmin;
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

            // step from the the start and the perpendicular length point p
            // until you hit the wall, move back and call that the hit location
            const float tmove = dot / RAY_BOX_HIT_SAMPLE_COUNT;
            float t = 0;

            glm::vec3 pt;
            for(int i = 0; i < RAY_BOX_HIT_SAMPLE_COUNT; ++i)
			{
				pt = start + dir * t;
                t += tmove;

				if (PointInBox(pt, pos, min, max))
                {
                    pt -= dir;
					break;
                }
            };

            *hit = pt;
        }

        return true;
    }

    return false;
#endif
}

// TODO: Refactor this so it returns a bool and takes a Hit& or something
// typeMask is a bitmask of entity types (created using ET_MASK(ET_PLAYER) | ET_MASK(ET_PAINTING)
static Entity* GetHitEntity(float x, float y, float z, float angle, Game& game, int typeMask, EntityType& type, glm::vec3& hitPos)
{
	struct Hit
	{
		EntityType etype;
		Entity* e;
        float t;
	};

    int entityCount = 0;

    static EntityType types[GAME_MAX_HIT_CHECK_ENTS];
    static Entity* entities[GAME_MAX_HIT_CHECK_ENTS];

    int hitCount = 0;
	static Hit hits[GAME_MAX_HIT_CHECK_ENTS];
    
    // TODO: Prune more entities which shouldn't be checked

    glm::vec3 start{x, y, z};
    glm::vec3 dir{sinf(angle), 0, cosf(angle)}; 

    if(typeMask & ET_MASK(ET_DOOR))
    {
        for(int i = 0; i < game.doorCount; ++i)
        {
            if(entityCount >= COUNT_OF(entities)) break;
            types[entityCount] = ET_DOOR;
            entities[entityCount++] = &game.doors[i];
        }
    }

    if(typeMask & ET_MASK(ET_ENEMY))
    {
        for(int i = 0; i < game.enemyCount; ++i)
        {
            if(entityCount >= COUNT_OF(entities)) break;
            if(game.enemies[i].health <= 0) continue;
            types[entityCount] = ET_ENEMY;
            entities[entityCount++] = &game.enemies[i];
        }
    }

    if(typeMask & ET_MASK(ET_PAINTING))
    {
        for(int i = 0; i < game.paintingCount; ++i)
        {
            if(entityCount >= COUNT_OF(entities)) break;
            types[entityCount] = ET_PAINTING;
            entities[entityCount++] = &game.paintings[i];
        }
    }

    if(typeMask & ET_MASK(ET_BOXCOLLIDER))
    {
        for(int i = 0; i < game.boxColliderCount; ++i)
        {
            if(entityCount >= COUNT_OF(entities)) break;

            types[entityCount] = ET_BOXCOLLIDER;
            entities[entityCount++] = &game.boxColliders[i];
        }
    }

    for(int i = 0; i < entityCount; ++i)
    {
        Entity& ent = *entities[i];
		float t;
        if(RayBox(start, dir, glm::vec3(ent.x, ent.y, ent.z), ent.min, ent.max, &t))
        {
            if(hitCount >= GAME_MAX_HIT_CHECK_ENTS) break;

            hits[hitCount++] = { types[i], &ent, t };
        }
    }

    // TODO: Better priority system; perhaps sort by type on top of distance?
    std::sort(hits, hits + hitCount, [](const Hit& a, const Hit& b) {
        return a.t < b.t;
    });

    if(hitCount > 0)
    {
		type = hits[0].etype;
        hitPos = start + dir * (hits[0].t - IMPACT_HOVER_EPSILON);
		return hits[0].e;
    }

    return nullptr;
}

static void Shoot(float x, float y, float z, float angle, Game& game)
{
    //CreateTracer(game, x, y + TRACER_Y_OFF, z, angle);

    EntityType etype;
    glm::vec3 hitPos;
    Entity* ent = GetHitEntity(x, y, z, angle, game, ET_MASK(ET_DOOR) | ET_MASK(ET_ENEMY) | ET_MASK(ET_PAINTING) | ET_MASK(ET_BOXCOLLIDER), etype, hitPos);

    if(ent)
    {
        if(etype == ET_ENEMY)
        {
            Enemy& enemy = *(Enemy*)ent;

            enemy.health -= 1;

            if(enemy.health <= 0)
            {
                enemy.state = Enemy::DEAD;
                enemy.animTimer = 0;
            }
            else
                enemy.hitTimer = ENEMY_HIT_TIME;
        }
        else if(etype == ET_PAINTING)
        {
            Painting& painting = *(Painting*)ent;

            painting.hit = true;
            painting.angularVel += ((float)rand() / RAND_MAX) - 0.5f;
        }
        else if(etype == ET_BOXCOLLIDER)
        {
            // Get closest tile center
            glm::vec3 c = hitPos;

            int tx = (int)(c.x / LEVEL_SCALE_FACTOR);
            int tz = (int)(c.z / LEVEL_SCALE_FACTOR);
            
            c = glm::vec3(tx * LEVEL_SCALE_FACTOR + LEVEL_SCALE_FACTOR / 2.0f, 0, tz * LEVEL_SCALE_FACTOR + LEVEL_SCALE_FACTOR / 2.0f);

            int dir = VecToDir(hitPos.x - c.x, hitPos.z - c.z, true);
            CreateImpact(game, hitPos.x, hitPos.y, hitPos.z, dir);
        }
    }
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

static bool CollideSolids(const Entity& e, float x, float y, float z, int typeMask, const Game& game)
{
    // TODO: Skip all entities that match e of every time

    if(typeMask & ET_MASK(ET_ENEMY))
    {
        for(int i = 0; i < game.enemyCount; ++i)
        {
            if(&e == &game.enemies[i]) continue;
            if(Collide(e, x, y, z, game.enemies[i]))
                return true;
        }
    }

    if(typeMask & ET_MASK(ET_BOXCOLLIDER))
    {
        for(int i = 0; i < game.boxColliderCount; ++i)
        {
            if(Collide(e, x, y, z, game.boxColliders[i]))
                return true;
        }
    }
    
    if(typeMask & ET_MASK(ET_DOOR))
    {
        for(int i = 0; i < game.doorCount; ++i)
        {
            if(Collide(e, x, y, z, game.doors[i]))
                return true;
        }
    }

    if(typeMask & ET_MASK(ET_PAINTING))
    {
        for(int i = 0; i < game.paintingCount; ++i)
        {
            if(Collide(e, x, y, z, game.paintings[i]))
                return true;
        }
    }

    return false;
}

static void MoveBy(Entity& e, float x, float y, float z, int typeMask, const Game& game)
{
    // TODO: Clean this up so it's not doing float cmp
    if(x != 0)
    {
        if(CollideSolids(e, e.x + x, e.y, e.z, typeMask, game)) x = 0;
        e.x += x;
    }

    if(y != 0)
    {
        if(CollideSolids(e, e.x, e.y + y, e.z, typeMask, game)) y = 0;
        e.y += y;
    }
    
    if(z != 0)
    {
        if(CollideSolids(e, e.x, e.y, e.z + z, typeMask, game)) z = 0;
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
        if(player.animTimer < 0.2f) 
        {
            player.animTimer += dt;
            player.frame = (int)(player.animTimer / 0.04f);
            
            // FIXME: We don't shoot from player.y because
            // that bobs below the bounding boxes of enemies
            // Maybe we shouldn't move the player y pos directly
            // and just render with the bob
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
        player.shoot = true; 

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
        MoveBy(player, x, 0, z, ET_MASK(ET_BOXCOLLIDER) | ET_MASK(ET_DOOR) | ET_MASK(ET_PAINTING), game);
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
    // TODO: Get rid of hitTimer and make it a state
    if(enemy.hitTimer > 0)
    {
        enemy.hitTimer -= dt;
        enemy.frame = 8 * 5 + 7;
        return;
    }

    glm::vec3 pdiff = Pos(game.player) - Pos(enemy);

    float angleDiff = atan2f(pdiff.x, pdiff.z);

    int dir = VecToDir(sinf(angleDiff - enemy.lookAngle), cosf(angleDiff - enemy.lookAngle));
    
    // Animations
    switch(enemy.state)
    {
        case Enemy::START_CHASE:
        case Enemy::IDLE:
        {
            enemy.frame = dir;
        } break;

        case Enemy::SAW_PLAYER:
        {
			enemy.frame = 8 * 6;
        } break;

        case Enemy::DEAD:
        {
            int deathFrame = ((int)(enemy.animTimer / 0.1f));
            if(deathFrame > 4) deathFrame = 4;

            enemy.frame = 8 * 5 + deathFrame;
        } break;

        case Enemy::WALKING:
        case Enemy::CHASING:
        {
            int walkFrame = ((int)(enemy.animTimer / 0.2f)) % 4; 
            enemy.frame = dir + (1 + walkFrame) * 8;
        } break;

        case Enemy::SHOOTING:
        {
            int shootFrame = ((int)(enemy.animTimer / 0.2f)) % 2;
            enemy.frame = 8 * 6 + 1 + shootFrame;
        } break;
    }

    // Updates
    switch(enemy.state)
    {
        case Enemy::WALKING:
        case Enemy::CHASING:
        {
            if(enemy.state == Enemy::CHASING)
                enemy.lookAngle = angleDiff;

            float mx, mz;
            Forward(enemy.lookAngle, mx, mz, dt * enemy.speed);

            MoveBy(enemy, mx, 0, mz, ET_MASK(ET_BOXCOLLIDER) | ET_MASK(ET_ENEMY) | ET_MASK(ET_DOOR) | ET_MASK(ET_PAINTING), game);
        } break;

        case Enemy::SAW_PLAYER:
        {
            enemy.lookAngle = angleDiff;
        } break;

        case Enemy::SHOOTING:
        {
            // TODO: Actually shoot at the player
        } break;
    }

    // State transitions
    switch(enemy.state)
    {
        case Enemy::IDLE:
        case Enemy::WALKING:
        {
            // TODO: Look at the absolute difference between the angle which
            // the enemy is looking in and angleDiff and check if that's under
            // some threshold
            
            if(glm::length2(Pos(game.player) - Pos(enemy)) < ENEMY_SIGHT_DIST * ENEMY_SIGHT_DIST)
            {
                EntityType etype;
				glm::vec3 t;
                // Make sure nothing between us and the player first
                Entity* e = GetHitEntity(enemy.x, enemy.y, enemy.z, angleDiff, game, ET_MASK(ET_DOOR) | ET_MASK(ET_BOXCOLLIDER), etype, t);
                if(!e || glm::length2(t - Pos(enemy)) >= glm::length2(Pos(game.player) - Pos(enemy)))
                {
                    enemy.lookAngle = angleDiff;
                    enemy.stateTimer = 0;
                    enemy.state = Enemy::SAW_PLAYER;	
                }
            }

			if (enemy.state == Enemy::IDLE && enemy.stateTimer >= ENEMY_IDLE_TIME)
			{
                enemy.lookAngle += ((((float)rand() / RAND_MAX) - 0.5f) * (float)M_PI);
				enemy.stateTimer = 0;
				enemy.state = Enemy::WALKING;
			}

            if(enemy.state == Enemy::WALKING && enemy.stateTimer >= ENEMY_WALK_TIME)
            {
                enemy.stateTimer = 0;
                enemy.state = Enemy::IDLE;
            }
        } break;

        case Enemy::SAW_PLAYER:
        {
            if(enemy.stateTimer >= ENEMY_SAW_PLAYER_TIME)
            {
                enemy.stateTimer = 0;
                enemy.state = Enemy::START_CHASE;
            }
        } break;

        case Enemy::START_CHASE:
        {
            if(enemy.stateTimer >= ENEMY_START_CHASE_TIME)
            {
                enemy.stateTimer = 0;
                enemy.state = Enemy::CHASING;
            }
        } break;

        case Enemy::CHASING:
        {
            float dist2 = glm::length2(Pos(game.player) - Pos(enemy));

            if(dist2 < ENEMY_SHOOT_DIST * ENEMY_SHOOT_DIST)
            {
                enemy.stateTimer = 0;
                enemy.state = Enemy::SHOOTING;
            }
            else if(dist2 >= ENEMY_LOSE_SIGHT_DIST * ENEMY_LOSE_SIGHT_DIST)
            {
                // TODO: Add a state to be looking for the player
                enemy.stateTimer = 0;
                enemy.state = Enemy::IDLE;
            }
        } break;

        case Enemy::SHOOTING:
        {
            if(glm::length2(Pos(game.player) - Pos(enemy)) > ENEMY_SHOOT_DIST * ENEMY_SHOOT_DIST)
            {
                enemy.stateTimer = 0;
                enemy.state = Enemy::START_CHASE;
            }
        } break;
    }

    enemy.stateTimer += dt;
    enemy.animTimer += dt;
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
    game.debugDraw = false;

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
        enemy.min = glm::vec3(-0.3f, -1.0f, -0.3f);
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
    if(WasKeyPressed(SDL_SCANCODE_TAB))
        game.debugDraw = !game.debugDraw;

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
        // Draw then facing the player plane
        glm::mat4 rot = glm::rotate(game.player.lookAngle - (float)M_PI, glm::vec3(0.0f, 1.0f, 0.0f));

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
        float rads = glm::radians(game.paintings[i].dir * DIR_DEGREES);
    
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

        glm::mat4 rot = glm::rotate(glm::radians(game.impacts[i].dir * DIR_DEGREES), glm::vec3(0, 1, 0)) * glm::translate(glm::vec3(-0.070f, -0.070f, 0));

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

    if(game.debugDraw)
    {
        // Draw box colliders
        glDisable(GL_DEPTH_TEST);

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

        for (int i = 0; i < game.enemyCount; ++i)
        {
            glm::vec3 min = game.enemies[i].min;
            glm::vec3 max = game.enemies[i].max;

            glm::mat4 scale = glm::scale(glm::vec3(max.x - min.x,
                                                   max.y - min.y,
                                                   max.z - min.z));
            
            glm::mat4 model = glm::translate(glm::vec3(game.enemies[i].x, game.enemies[i].y, game.enemies[i].z)) * scale;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            Draw(game.boxMesh);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glEnable(GL_DEPTH_TEST);
    }

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
