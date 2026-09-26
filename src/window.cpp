#include "headers/window.h"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_impl_sdl3.h"
#include "../vendor/imgui/imgui_impl_sdlrenderer3.h"

struct SDLWindowState;

int InitSDL()
{
  if(!SDL_Init(SDL_INIT_VIDEO))
    {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error when initializing SDL3", nullptr);
      return 1;
    }
  return 0;
}

int CreateWindowAndRenderer(const char *title, SDL_WindowFlags flags, SDLWindowState &state)
{
  if (!SDL_CreateWindowAndRenderer(title, state.width, state.height, flags, &state.window, &state.renderer))
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
      return 1;
    }
  return 0;
}

int CreateWindow(const char *title, SDL_WindowFlags flags, SDLWindowState &state)
{
  state.window = SDL_CreateWindow(title, state.width, state.height, flags);
  if (!state.window)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s", SDL_GetError());
      return 1;
    }
  return 0;
}

void LoopHandler(SDLWindowState state, glm::vec4 backgroundColor)
{
  LoopState loopState = LoopState::RUNNING;

  // Init ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; //Enable Keyboard
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //Enable Gamepad

  ImGui::StyleColorsDark();
  io.Fonts->AddFontDefault();
  
  ImGui_ImplSDLRenderer3_Init(state.renderer);
  ImGui_ImplSDL3_InitForSDLRenderer(state.window, state.renderer);

  while(loopState != LoopState::QUIT)
  {
    SDL_Event event { 0 };
    while (SDL_PollEvent(&event))
    {
      // Poll events for ImGui
      ImGui_ImplSDL3_ProcessEvent(&event);
      EventHandler(&state, event, loopState);
    }
    DrawBackground(state, backgroundColor);

    // Start ImGui frame
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();

    // Show Demo Frame
    ImGui::ShowDemoWindow();

    // Render Imgui Frame
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), state.renderer);

    //swap buffers and show to screen
    SDL_RenderPresent(state.renderer);
  }
  // Clean ImGui
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  CleanUp(state);
}

void KeyboardHandler(SDL_Event &event, LoopState &loopState)
{
  switch (event.key.key) 
  {
    case SDLK_Q:
      loopState = LoopState::QUIT;
    break;
  }
}

void MouseHandler(SDL_Event &event)
{
  switch (event.button.button) 
  {
    case SDL_BUTTON_LEFT:
      SDL_Log("Left Click");
    break;
    case SDL_BUTTON_MIDDLE:
      SDL_Log("Middle Click");
    break;
    case SDL_BUTTON_RIGHT:
      SDL_Log("Right Click");
    break;
  }
}

void EventHandler(SDLWindowState *state, SDL_Event &event, LoopState &loopState)
{
  switch (event.type)
  {
    case SDL_EVENT_QUIT:
      loopState = LoopState::QUIT;
    break;
    case SDL_EVENT_WINDOW_RESIZED:
      state->width = event.window.data1;
      state->height = event.window.data2;
    break;
    case SDL_EVENT_KEY_UP:
      KeyboardHandler(event, loopState);
    break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      MouseHandler(event);
    break;
  }
}

void DrawBackground(SDLWindowState state, glm::vec4 backgroundColor)
{
  SDL_SetRenderDrawColorFloat(state.renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
  SDL_RenderClear(state.renderer);
}

void CleanUp(SDLWindowState &state)
{
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(state.window);
  SDL_Quit();
}
