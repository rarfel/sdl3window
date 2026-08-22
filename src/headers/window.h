#pragma once
#include "SDL3/SDL.h"

struct SDLWindowState
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  int width;
  int height;
};

int InitSDL();

int CreateWindowAndRenderer(const char *title, SDL_WindowFlags flags, SDLWindowState &state);

void CleanUp(SDLWindowState &state);
