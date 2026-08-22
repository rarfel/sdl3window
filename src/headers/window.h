#pragma once
#include "SDL3/SDL.h"

struct SDLWindowState
{
  SDL_Window *window;
  SDL_Renderer *renderer;
};

int InitSDL();

int CreateWindowAndRenderer(const char *title, int width, int height, SDL_WindowFlags flags, SDLWindowState &state);

void CleanUp(SDLWindowState &state);
