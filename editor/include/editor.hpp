#pragma once

#include "resources.hpp"

union SDL_Event;
struct Context;

struct TilePanel
{
    int tile = 0;
};

struct Tilemap
{
    int width = 0, height = 0;
    int* tiles = nullptr;
};

struct Editor
{
    bool commandMode = false;

    TilePanel tilePanel;
    Tilemap tilemap;

    const Context* context = nullptr;

    mutable Texture tileTexture;
    mutable Font font;
};

void Init(Editor& editor, const Context& context);

void HandleEvent(Editor& editor, const SDL_Event& event);
void Update(Editor& editor, float dt);
void Draw(const Editor& editor);
