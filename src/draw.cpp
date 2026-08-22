#include "headers/draw.h"

struct Color;

void DrawBackground(SDLWindowState state, Color backgroundColor)
{
  SDL_SetRenderDrawColorFloat(state.renderer, backgroundColor.red, backgroundColor.green, backgroundColor.blue, backgroundColor.alpha);
  SDL_RenderClear(state.renderer);
}
