#include "headers/draw.h"

template <typename T>
struct Color;

void DrawBackground(SDLWindowState state, Color<float> backgroundColor)
{
  SDL_SetRenderDrawColorFloat(state.renderer, backgroundColor.red, backgroundColor.green, backgroundColor.blue, backgroundColor.alpha);
  SDL_RenderClear(state.renderer);
}
