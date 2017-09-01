#include <gl3w.h>

#include "context.hpp"
#include "utils.hpp"

inline static void ScreenToViewCoords(const Context& context, int screenX, int screenY, float& viewX, float& viewY)
{
    if(context.scaleMode == CONTEXT_SCALE_ASPECT)
    {
        int sw, sh;
        SDL_GetWindowSize(context.window, &sw, &sh);

        float aspect = 4 / 3.0f;
        
        float scaledWidth = sh * aspect;
        float offX = (sw - scaledWidth) / 2.0f;

        viewX = ((screenX - offX) / (float)sw) * context.viewWidth;
        viewY = (screenY / (float)sh) * context.viewHeight;
    }
    else if(context.scaleMode == CONTEXT_SCALE_GROW)
    {
        viewX = (float)screenX;
        viewY = (float)screenY;
    }
    else if(context.scaleMode == CONTEXT_SCALE_STRETCH)
    {
        int sw, sh;
        SDL_GetWindowSize(context.window, &sw, &sh);

        viewX = (screenX / (float)sw) * context.viewWidth;
        viewY = (screenY / (float)sh) * context.viewHeight;
    }
}

Context CreateContext(const char* title, int width, int height, ContextScaleMode scaleMode)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
		CRASH("Failed to initialize SDL2: %s\n", SDL_GetError());

	atexit(SDL_Quit);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window = SDL_CreateWindow(title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	if (!window)
		CRASH("Failed to create SDL_Window: %s\n", SDL_GetError());

	SDL_GLContext glContext = SDL_GL_CreateContext(window);

	if (!glContext)
		CRASH("Failed to create SDL_GLContext: %s\n", SDL_GetError());

	if (gl3wInit())
		CRASH("Failed to initialize gl3w\n");

    SDL_GL_SetSwapInterval(-1);

    Context context;

    context.scaleMode = scaleMode;
    context.viewWidth = (float)width;
    context.viewHeight = (float)height;
    context.window = window;
    context.glContext = glContext;

    return context;
}

void HandleEvent(Context& context, SDL_Event& event)
{
    if(event.type == SDL_WINDOWEVENT)
    {
        if(event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            int width = event.window.data1;
            int height = event.window.data2;

            if(context.scaleMode == CONTEXT_SCALE_ASPECT)
            {
                float newWidth = height * (4 / 3.0f);
                float newX = (width - newWidth) / 2.0f;

                glViewport(newX, 0, newWidth, event.window.data2);
            }
            else if(context.scaleMode == CONTEXT_SCALE_STRETCH)
                glViewport(0, 0, width, height);
            else if(context.scaleMode == CONTEXT_SCALE_GROW)
            {
                context.viewWidth = (float)width;
                context.viewHeight = (float)height;

                glViewport(0, 0, width, height);
            }
        }
    }
    else if(event.type == SDL_MOUSEMOTION)
    {
        float viewX, viewY;
        ScreenToViewCoords(context, event.motion.x, event.motion.y, viewX, viewY);

        event.motion.x = (int)viewX;
        event.motion.y = (int)viewY;
    }
    else if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
    {
        float viewX, viewY;
        ScreenToViewCoords(context, event.button.x, event.button.y, viewX, viewY);

        event.button.x = (int)viewX;
        event.button.y = (int)viewY; 
    }
}

void DestroyContext(Context& context)
{
    SDL_GL_DeleteContext(context.glContext);
    SDL_DestroyWindow(context.window);
}
