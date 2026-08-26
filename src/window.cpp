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

void DrawBackground(SDLWindowState state, glm::vec4 backgroundColor)
{
  SDL_SetRenderDrawColorFloat(state.renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
  SDL_RenderClear(state.renderer);
}

void CleanUp(SDLWindowState &state)
{
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(state.window);
  SDL_Quit();
}
