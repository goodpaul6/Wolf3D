#include <gl3w.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.hpp"
#include "game.hpp"
#include "input.hpp"
#include "context.hpp"

static const int WINDOW_WIDTH = 640;
static const int WINDOW_HEIGHT = 480;

int main(int argc, char** argv)
{
    Context context = CreateContext("Wolf3D", WINDOW_WIDTH, WINDOW_HEIGHT, CONTEXT_SCALE_STRETCH);

	SDL_Event event;
	bool running = true;

	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // TODO: Enable back-face culling and handle walls properly

    Game game;
    
    Init(game);

    Uint64 ticks = SDL_GetPerformanceCounter();
    
    glClearColor(0, 0, 0, 1.0f);
	
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

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), context.viewWidth / context.viewHeight, 0.2f, 100.0f);

        Draw(game, proj);

        SDL_GL_SwapWindow(context.window);
    }

    DestroyContext(context);

    return 0;
}
