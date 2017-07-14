#pragma once

#include <SDL.h>

void UpdateInput();

bool IsKeyDown(SDL_Scancode scancode);
bool WasKeyPressed(SDL_Scancode scancode);

