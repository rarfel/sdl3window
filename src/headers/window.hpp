#pragma once
#include "SDL3/SDL.h"
#include "glm/glm.hpp"

struct SDLWindowState
{
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  float width = -1;
  float height = -1;
};

enum class LoopState
{
  QUIT,
  RUNNING,
};

int InitSDL();

bool CreateWindowAndRenderer(const char *title, SDL_WindowFlags flags, SDLWindowState &state);
bool CreateWindow(const char *title, SDL_WindowFlags flags, SDLWindowState &state);

void LoopHandler(SDLWindowState state, glm::vec4 backgroundColor);
void EventHandler(SDLWindowState *state, SDL_Event &event, LoopState &loopState);

void KeyboardHandler(SDL_Event &event, LoopState &loopState);
void MouseHandler(SDL_Event &event);

void DrawBackground(SDLWindowState state, glm::vec4 backgroundColor);

void CleanUp(SDLWindowState &state);
