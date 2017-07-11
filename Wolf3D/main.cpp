#include <GL/gl3w.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "resources.hpp"
#include "graphics.hpp"
#include "utils.hpp"

struct Player
{
    float x = 0, y = 0, z = 0;
    float lookAngle = 0;         // radians
};

static const int WINDOW_WIDTH = 640;
static const int WINDOW_HEIGHT = 480;

inline static bool IsKeyDown(SDL_Scancode scancode)
{
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    return keys[(int)scancode];
}

int main(int argc, char** argv)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
		CRASH("Failed to initialize SDL2: %s\n", SDL_GetError());

	atexit(SDL_Quit);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window = SDL_CreateWindow("Wolf 3D",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	if (!window)
		CRASH("Failed to create SDL_Window: %s\n", SDL_GetError());

	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!context)
		CRASH("Failed to create SDL_GLContext: %s\n", SDL_GetError());

	if (gl3wInit())
		CRASH("Failed to initialize gl3w\n");

    Level level = LoadLevel("levels/test.map");

    Texture texture = LoadTexture("textures/wolf.png");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);

	SDL_Event event;
	bool running = true;

	glm::mat4 proj = glm::perspective(glm::radians(45.0f), WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.05f, 100.0f);

    Shader shader = LoadShader("shaders/basic.vert", "shaders/basic.frag");

	GLuint modelLoc = glGetUniformLocation(shader.id, "model");
	GLuint viewLoc = glGetUniformLocation(shader.id, "view");
    GLuint projLoc = glGetUniformLocation(shader.id, "proj");
    
    GLuint textureLoc = glGetUniformLocation(shader.id, "tex");

	glUseProgram(shader.id);

    glUniform1i(textureLoc, 0);

    Mesh levelMesh = CreateLevelMesh(level, texture);

	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

	/*glEnable(GL_CULL_FACE);

	glFrontFace(GL_CW);
	glCullFace(GL_BACK);*/

	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

    Player player;
	float playerStride = 0;

    player.z = 2.0f;

    Uint64 ticks = SDL_GetPerformanceCounter();

    glClearColor(0.1f, 0.1f, 0.1f, 1);

    while(running)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                running = false;
            else if(event.type == SDL_WINDOWEVENT)
            {
                if(event.window.event == SDL_WINDOWEVENT_RESIZED)
                    glViewport(0, 0, event.window.data1, event.window.data2);
            }
        }

        Uint64 elapsed = SDL_GetPerformanceCounter() - ticks;
        ticks = SDL_GetPerformanceCounter();

        float dt = elapsed / (float)SDL_GetPerformanceFrequency();

        if(IsKeyDown(SDL_SCANCODE_A))
            player.lookAngle += 2 * dt;
        else if(IsKeyDown(SDL_SCANCODE_D))
            player.lookAngle -= 2 * dt;

        if(IsKeyDown(SDL_SCANCODE_W))
        {
            player.x += sinf(player.lookAngle) * 5 * dt;
            player.z += cosf(player.lookAngle) * 5 * dt;
            player.y = sinf(playerStride) / 20.0f;

            playerStride += 10 * dt;
        }
        else if(IsKeyDown(SDL_SCANCODE_S))
        {
            player.x -= sinf(player.lookAngle) * 5 * dt;
            player.z -= cosf(player.lookAngle) * 5 * dt;
            player.y = sinf(playerStride) / 20.0f;

            playerStride += 10 * dt;
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(glm::vec3(player.x, player.y, player.z), 
                                     glm::vec3(player.x + sinf(player.lookAngle), player.y, player.z + cosf(player.lookAngle)), 
                                     glm::vec3(0, 1, 0));

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        
#if 0
		glm::mat4 model;
		model = glm::rotate(model, glm::radians(SDL_GetTicks() / 10.0f), glm::vec3(0, 0, 1));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_TRIANGLES, 0, sizeof(PLANE_VERTEX_DATA) / (sizeof(float) * 3));

        model = glm::translate(glm::vec3(0, -1, 0)) * glm::scale(glm::vec3(10, 1, 10)) * glm::rotate(glm::radians(-90.0f), glm::vec3(1, 0, 0));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_TRIANGLES, 0, sizeof(PLANE_VERTEX_DATA) / POS_TEX_VERTEX_SIZE);
#endif

        glm::mat4 model = glm::translate(glm::vec3(-2, -1, -2));
		//glm::mat4 model;
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        
        Draw(levelMesh);

        SDL_GL_SwapWindow(window);
    }

    DestroyMesh(levelMesh);
    DestroyLevel(level);

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);

    return 0;
}
