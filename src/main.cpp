#include "headers/window.h"
#include "SDL3/SDL_main.h"

int main(int argc, char *argv[])
{
  SDLWindowState state;

  InitSDL();

  // init window
  int width = 800;
  int height = 600;

  CreateWindowAndRenderer("SDL3Window", width, height, SDL_WINDOW_RESIZABLE, state);

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
  CleanUp(state);
}
