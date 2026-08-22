#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

struct SDLWindowState
{
  SDL_Window *window;
  SDL_Renderer *renderer;
};

void cleanUp(SDLWindowState &state);

int main(int argc, char *argv[])
{
  SDLWindowState state;

  if(!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error when initializing SDL3", nullptr);
    return 1;
  }

  // init window
  int width = 800;
  int height = 600;
  state.window = SDL_CreateWindow("SDL3Window", width, height,0);

  if(!state.window)
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error when creating window", nullptr);
    cleanUp(state);
    return 1;
  }

  // init renderer
  state.renderer = SDL_CreateRenderer(state.window, nullptr);
  if(!state.renderer)
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error when creating renderer", state.window);
    cleanUp(state);
    return 1;
  }

  // game loop
  bool running = true;
  while(running)
  {
    SDL_Event event { 0 };
    while(SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_EVENT_QUIT:
        {
          running = false;
          break;
        }
      }
    }
    // Drawing commands
    SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
    SDL_RenderClear(state.renderer);
    //swap buffers and show to screen
    SDL_RenderPresent(state.renderer);
  }
  cleanUp(state);
}

void cleanUp(SDLWindowState &state)
{
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(state.window);
  SDL_Quit();
}
