#include <gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "context.hpp"

#include "editor.hpp"
#include "draw.hpp"

#undef main

static const int WINDOW_WIDTH = 640;
static const int WINDOW_HEIGHT = 480;

int main(int argc, char** argv)
{
    Context context = CreateContext("Wolf3D Level Editor", WINDOW_WIDTH, WINDOW_HEIGHT, CONTEXT_SCALE_GROW);
    
    InitDraw(WINDOW_WIDTH, WINDOW_HEIGHT);

    glClearColor(0.05f, 0.05f, 0.05f, 1);

    SDL_Event event;
    bool running = true;

    Uint64 ticks = SDL_GetPerformanceCounter();

    Editor editor;

    Init(editor, context);
    
    while(running)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                running = false;

            HandleEvent(context, event);
            HandleEvent(editor, event);
        }

        Uint64 elapsed = SDL_GetPerformanceCounter() - ticks;
        ticks = SDL_GetPerformanceCounter();

        float dt = elapsed / (float)SDL_GetPerformanceFrequency();

        Update(editor, dt);

        glClear(GL_COLOR_BUFFER_BIT);

        SetViewSize(context.viewWidth, context.viewHeight);
        Draw(editor);
   
        SDL_GL_SwapWindow(context.window);
    }

    DestroyDraw();
	DestroyContext(context);

    return 0;
}

