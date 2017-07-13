#pragma once

#include <SDL.h>

inline static const Uint8 IsKeyDown(SDL_Scancode scancode)
{
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    return keys[(int)scancode];
}
