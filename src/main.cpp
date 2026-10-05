#include "headers/window.hpp"
#include "SDL3/SDL_main.h"

int main(int argc, char *argv[])
{
  SDLWindowState state;
  state.width = 800;
  state.height = 600;

  InitSDL();

  CreateWindow("SDL3Window", SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE, state);

  glm::vec4 backgroundColor = {0.0, 0.0, 0.0, 1.0};

  LoopHandler(state, backgroundColor);
}
