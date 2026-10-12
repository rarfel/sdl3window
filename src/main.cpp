#include "headers/window.hpp"
#include "SDL3/SDL_main.h"

int main(int argc, char *argv[])
{
  Window window("SDL3Window", SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE, 800, 600);

  window.CreateWindowAndRenderer();

  glm::vec4 backgroundColor = {0.0, 0.0, 0.0, 1.0};

  window.LoopHandler(backgroundColor);
}
