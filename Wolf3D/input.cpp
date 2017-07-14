#include <string.h>

#include "input.hpp"

static Uint8 PrevKeyState[SDL_NUM_SCANCODES];
static Uint8 KeyState[SDL_NUM_SCANCODES];

void UpdateInput()
{
    int keyCount = 0;
    const Uint8* keys = SDL_GetKeyboardState(&keyCount);

    memcpy((void*)PrevKeyState, (void*)KeyState, sizeof(PrevKeyState));
    memcpy((void*)KeyState, (void*)keys, sizeof(Uint8) * keyCount);
}

bool IsKeyDown(SDL_Scancode scancode)
{
    return KeyState[(int)scancode] > 0;
}

bool WasKeyPressed(SDL_Scancode scancode)
{
    return PrevKeyState[(int)scancode] == 0 && KeyState[(int)scancode] > 0;
}

