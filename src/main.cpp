#include "headers/draw.h"
#include "SDL3/SDL_main.h"

int main(int argc, char *argv[])
{
  SDLWindowState state;

  InitSDL();

  // init window
  int width = 800;
  int height = 600;

  CreateWindowAndRenderer("SDL3Window", width, height, SDL_WINDOW_RESIZABLE, state);

  Color backgroundColor = {0.0, 0.0, 0.0, 1.0};
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

    const double now = ((double)SDL_GetTicks()) / 1000.0;

    backgroundColor.red = (float) (0.5 + 0.5 * SDL_sin(now));
    backgroundColor.green = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2/3));
    backgroundColor.blue = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D) * 4/3);
  }
  CleanUp(state);
}
