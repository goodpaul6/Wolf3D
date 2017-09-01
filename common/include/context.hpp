#pragma once

#include <SDL.h>

enum ContextScaleMode
{
    CONTEXT_SCALE_GROW,
    CONTEXT_SCALE_ASPECT,
    CONTEXT_SCALE_STRETCH
};

struct Context
{
    ContextScaleMode scaleMode;
    float viewWidth, viewHeight;
    SDL_Window* window;
    SDL_GLContext glContext;
};

Context CreateContext(const char* title, int width, int height, ContextScaleMode scaleMode);

// Handles resizing the opengl viewport and updating
// mouse coordinates in the SDL_Event structure to 
// view coordinates
void HandleEvent(Context& context, SDL_Event& event);

void DestroyContext(Context& context);


