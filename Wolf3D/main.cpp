#include <GL/gl3w.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.hpp"
#include "game.hpp"
#include "input.hpp"

static const int WINDOW_WIDTH = 640;
static const int WINDOW_HEIGHT = 480;

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

    SDL_GL_SetSwapInterval(-1);

	SDL_Event event;
	bool running = true;

	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // TODO: Enable back-face culling and handle walls properly

    Game game;
    
    Init(game);

    Uint64 ticks = SDL_GetPerformanceCounter();
    
    glClearColor(0, 0, 0, 1.0f);
	
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.05f, 100.0f);

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

        UpdateInput();

        Uint64 elapsed = SDL_GetPerformanceCounter() - ticks;
        ticks = SDL_GetPerformanceCounter();

        float dt = elapsed / (float)SDL_GetPerformanceFrequency();
        
        Update(game, dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Draw(game, proj);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);

    return 0;
}
