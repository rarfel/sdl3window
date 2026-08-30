#include "headers/window.h"

struct SDLWindowState;

int InitSDL()
{
  if(!SDL_Init(SDL_INIT_VIDEO))
    {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error when initializing SDL3", nullptr);
      return 1;
    }
  return 0;
}

int CreateWindowAndRenderer(const char *title, SDL_WindowFlags flags, SDLWindowState &state)
{
  if (!SDL_CreateWindowAndRenderer(title, state.width, state.height, flags, &state.window, &state.renderer))
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
      return 1;
    }
  return 0;
}

int CreateWindow(const char *title, SDL_WindowFlags flags, SDLWindowState &state)
{
  state.window = SDL_CreateWindow(title, state.width, state.height, flags);
  if (!state.window)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s", SDL_GetError());
      return 1;
    }
  return 0;
}

void LoopHandler(SDLWindowState state, glm::vec4 backgroundColor)
{
  bool running = true;
  while(running)
  {
    SDL_Event event { 0 };
    while (SDL_PollEvent(&event))
    {
      running = EventHandler(&state, event);
    }
    DrawBackground(state, backgroundColor);
  }
  CleanUp(state);
}

bool EventHandler(SDLWindowState *state, SDL_Event &event)
{
  switch (event.type)
  {
    case SDL_EVENT_QUIT:
    {
      return false;
      break;
    }
    case SDL_EVENT_WINDOW_RESIZED:
    {
      state->width = event.window.data1;
      state->height = event.window.data2;
      SDL_Log("X:%f|Y:%f", state->width,state->height);
      break;
    }
  }
  return true;
}

void DrawBackground(SDLWindowState state, glm::vec4 backgroundColor)
{
  SDL_SetRenderDrawColorFloat(state.renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
  SDL_RenderClear(state.renderer);
  //swap buffers and show to screen
  SDL_RenderPresent(state.renderer);
}

void CleanUp(SDLWindowState &state)
{
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(state.window);
  SDL_Quit();
}
