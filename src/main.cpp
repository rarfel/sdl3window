#include "headers/draw.h"
#include "SDL3/SDL_main.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
  SDLWindowState state;

  InitSDL();

  // init window
  int width = 800;
  int height = 600;

  CreateWindowAndRenderer("SDL3Window", width, height, 0, state);

  Color<float> backgroundColor = {0.0, 0.0, 0.0, 1.0};
  int *pWidth;
  int *pHeight;
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
    DrawBackground(state, backgroundColor);
    //swap buffers and show to screen
    SDL_RenderPresent(state.renderer);

  }
  CleanUp(state);
}
