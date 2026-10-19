#include "SDL3/SDL_main.h"
#include "headers/window.hpp"
#include "headers/vulkanRender.hpp"

int main(int argc, char *argv[])
{
  Window window("SDL3Window", SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN, 1000, 800);

  window.CreateWindow();

  glm::vec4 backgroundColor = {0.01, 0.01, 0.01, 1.0};

  window.LoopHandler(backgroundColor);
}
