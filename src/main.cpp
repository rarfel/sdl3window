#include "headers/draw.h"
#include "SDL3/SDL_main.h"

int main(int argc, char *argv[])
{
  SDLWindowState state;
  state.width = 800;
  state.height = 600;

  InitSDL();

  CreateWindowAndRenderer("SDL3Window", SDL_WINDOW_RESIZABLE, state);

  glm::vec4 backgroundColor = {0.0, 0.0, 0.0, 1.0};
  glm::ivec4 lineColor = {255, 255, 255, 255};

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

    // Funky background is back i guess
    const double now = ((double)SDL_GetTicks()) / 1000.0;
    backgroundColor.r = (float) (0.5 + 0.5 * SDL_sin(now));
    backgroundColor.g = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2/3));
    backgroundColor.b = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D) * 4/3);

    SDL_SetRenderDrawColor(state.renderer, lineColor.r, lineColor.g, lineColor.b, lineColor.a);
    // Making a square with lines just for show
    SDL_RenderLine(state.renderer, (state.width/5.0), (100), (state.width/5.0), (state.height - 100));
    SDL_RenderLine(state.renderer, (state.width/1.2), (100), (state.width/1.2), (state.height - 100));

    SDL_RenderLine(state.renderer, (state.width/5.0), (100), (state.width/1.2), (100));
    SDL_RenderLine(state.renderer, (state.width/5.0), (state.height - 100), (state.width/1.2), (state.height - 100));

    SDL_RenderLine(state.renderer, (state.width/5.0), (100), (state.width/1.2), (state.height - 100));
    SDL_RenderLine(state.renderer, (state.width/5.0), (state.height - 100), (state.width/1.2), (100));

    //swap buffers and show to screen
    SDL_RenderPresent(state.renderer);
  }
  CleanUp(state);
}
