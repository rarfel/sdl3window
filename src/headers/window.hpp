#pragma once
#include "SDL3/SDL.h"
#include "glm/glm.hpp"
#include <cstdint>

struct SDLWindowState
{
  SDL_Window *window = nullptr;
  uint32_t width = -1;
  uint32_t height = -1;
};

enum class LoopState
{
  QUIT,
  RUNNING,
};

class Window
{
  SDLWindowState state;
  LoopState loopState;
  const char *windowTitle;
  SDL_WindowFlags windowFlags;

  int InitSDL();

  void EventHandler(SDL_Event &event);

  void KeyboardHandler(SDL_Event &event, LoopState &loopState);
  void MouseHandler(SDL_Event &event);
  void DrawBackground(glm::vec4 backgroundColor);

  void CleanSDL();

  public:
  Window(const char *title, SDL_WindowFlags flags, uint32_t width, uint32_t height);
  ~Window();

  bool CreateWindow();

  void LoopHandler(glm::vec4 backgroundColor);
};
