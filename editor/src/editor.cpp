#include <SDL.h>

#include "utils.hpp"
#include "context.hpp"

#include "editor.hpp"
#include "draw.hpp"

static const float COMMAND_BOX_HEIGHT = 24.0f;

static const char* TILE_NAMES[] =
{
    "floor",
    "wood"
};

static int TILE_FRAMES[] =
{
    0, 22
};

inline static bool ContainsPoint(float x, float y, float w, float h, float px, float py)
{
    return px >= x && py >= y && px <= (x + w) && py <= (y + h);
}

void Init(Editor& editor, const Context& context)
{
    editor.tileTexture = LoadTexture("textures/wolf.png");
	editor.font = LoadFont("fonts/consola.ttf", 24.0f);

    editor.context = &context;
}

void HandleEvent(Editor& editor, const SDL_Event& event)
{
    if(event.type == SDL_KEYDOWN && 
        (event.key.keysym.sym == SDLK_SEMICOLON && (event.key.keysym.mod & MK_SHIFT)))
    {
        // Enter/exit command mode
        editor.commandMode = !editor.commandMode;

        if(editor.commandMode)
            SDL_StartTextInput();
        else
            SDL_StopTextInput();
    }

    if(event.type == SDL_TEXTINPUT)
    {

    }

    if(event.type == SDL_MOUSEBUTTONDOWN)
    {
        if(!editor.commandMode)
        {
            for(int i = 0; i < COUNT_OF(TILE_NAMES); ++i)
            {
                if(ContainsPoint(0, i * 64, 64, 64, event.button.x, event.button.y))
                    editor.tilePanel.tile = i;
            }
        }
    }
}

void Update(Editor& editor, float dt)
{
}

void Draw(const Editor& editor)
{
    if(editor.commandMode)
    {
        SetDrawColorNorm(100, 100, 100);
        FillRect(0, editor.context->viewHeight - COMMAND_BOX_HEIGHT, 
                editor.context->viewWidth, COMMAND_BOX_HEIGHT);
    }
    else
    {
        for(int i = 0; i < COUNT_OF(TILE_NAMES); ++i)
        {
            DrawFrame(editor.tileTexture, 4, i * 64 + 4, 64, 64, 64, 64, TILE_FRAMES[i]);
            
            if(i == editor.tilePanel.tile)
            {
                SetDrawColorNorm(255, 0, 0);
                DrawRect(4, i * 64 + 4, 64, 64);
            }
        }

         FillText(editor.font, 200, 200, "HELLO");
    }
}
