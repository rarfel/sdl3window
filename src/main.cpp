#include "headers/draw.h"
#include "SDL3/SDL_main.h"

int main(int argc, char *argv[])
{
  SDLWindowState state;
  state.width = 800;
  state.height = 600;

  InitSDL();

  CreateWindowAndRenderer("SDL3Window", SDL_WINDOW_RESIZABLE, state);

  Color<float> backgroundColor = {0.0, 0.0, 0.0, 1.0};
  Color<int> lineColor = {255, 0, 255, 255};
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
        case SDL_EVENT_WINDOW_RESIZED:
        {
          state.width = event.window.data1;
          state.height = event.window.data2;
          break;
        }
      }
    }
    // Drawing commands
    DrawBackground(state, backgroundColor);
    SDL_SetRenderDrawColor(state.renderer, lineColor.red, lineColor.green, lineColor.blue, lineColor.alpha);
    SDL_RenderLine(state.renderer, 0, 0, state.width, state.height);
    //swap buffers and show to screen
    SDL_RenderPresent(state.renderer);
  }
  CleanUp(state);
}
