#include "headers/window.hpp"
#include "headers/imguiMenu.hpp"
#include "headers/vulkanRender.hpp"

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

bool CreateWindowAndRenderer(const char *title, SDL_WindowFlags flags, SDLWindowState &state)
{
  if (!SDL_CreateWindowAndRenderer(title, state.width, state.height, flags, &state.window, &state.renderer))
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
      return false;
    }
  return true;
}

bool CreateWindow(const char *title, SDL_WindowFlags flags, SDLWindowState &state)
{
  state.window = SDL_CreateWindow(title, state.width, state.height, flags);
  if (!state.window)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s", SDL_GetError());
      return false;
    }
  return true;
}

void LoopHandler(SDLWindowState state, glm::vec4 backgroundColor)
{
  LoopState loopState = LoopState::RUNNING;
  bool demoMenu = true;

  VulkanRenderer renderer(state.window, state.width, state.height);
  if(!renderer.InitVulkan())
  {
    SDL_Log("Counld't init vulkan");
    return;
  }

  // Init ImGui
  //ImGuiIO *io = InitImGuiMenu(&ImGui_ImplSDL3_InitForSDLRenderer, state.window, state.renderer);

  while(loopState != LoopState::QUIT)
  {
    SDL_Event event { 0 };
    while (SDL_PollEvent(&event))
    {
      // Poll events for ImGui and SDL3
      //ImGui_ImplSDL3_ProcessEvent(&event);
      EventHandler(&state, event, loopState);
    }
    //DrawBackground(state, backgroundColor);

    /*StartImGuiFrame(&ImGui_ImplSDLRenderer3_NewFrame);

    // Create a menu with ImGui::Begin() --menu information-- ImGui::End()
    {
      ImGui::Begin("Hello, world!");

      ImGui::Text("This is some useful text.");
      ImGui::Checkbox("Demo Window", &demoMenu);

      ImGui::ColorEdit3("Background Color", (float*)&backgroundColor);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
      if(ImGui::Button("Close Window"))
      {
        loopState = LoopState::QUIT;
      }
      ImGui::End();
    }
    // Show Demo Frame
    if(demoMenu) 
      ImGui::ShowDemoWindow();

    RenderImGuiFrame(&ImGui_ImplSDLRenderer3_RenderDrawData, state.renderer);*/
    
    renderer.Render(backgroundColor);
    }

  //CleanImGuiMenu(&ImGui_ImplSDLRenderer3_Shutdown);
  renderer.CleanVulkan();
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
  SDL_DestroyWindow(state.window);
  SDL_Quit();
}
