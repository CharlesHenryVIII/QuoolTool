#pragma once
#include "Math.h"

#include "SDL3/SDL.h"

struct Renderer
{
    SDL_Window* window;
    Vec2I window_size;
};
extern Renderer gfx;

bool RenderInit();
