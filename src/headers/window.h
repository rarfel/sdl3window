#pragma once
#include "SDL3/SDL.h"
#include "glm/glm.hpp"

struct SDLWindowState
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  int width;
  int height;
};

int InitSDL();

int CreateWindowAndRenderer(const char *title, SDL_WindowFlags flags, SDLWindowState &state);

void DrawBackground(SDLWindowState state, glm::vec4 backgroundColor);

void CleanUp(SDLWindowState &state);
