#pragma once
#include "SDL3/SDL.h"
#include "glm/glm.hpp"

struct SDLWindowState
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  float width;
  float height;
};

int InitSDL();

int CreateWindowAndRenderer(const char *title, SDL_WindowFlags flags, SDLWindowState &state);
int CreateWindow(const char *title, SDL_WindowFlags flags, SDLWindowState &state);

void LoopHandler(SDLWindowState state, glm::vec4 backgroundColor);

bool EventHandler(SDLWindowState *state, SDL_Event &event);

void DrawBackground(SDLWindowState state, glm::vec4 backgroundColor);

void DrawBackground(SDLWindowState state, glm::vec4 backgroundColor);

void CleanUp(SDLWindowState &state);
