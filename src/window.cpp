#include "headers/window.hpp"
#include "headers/imguiMenu.hpp"
#include "headers/vulkanRender.hpp"

int Window::InitSDL()
{
  if(!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error when initializing SDL3", nullptr);
    return 1;
  }
  return 0;
}

bool Window::CreateWindow()
{
  state.window = SDL_CreateWindow(windowTitle, state.width, state.height, windowFlags);
  if (!state.window)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s", SDL_GetError());
    return false;
  }
  return true;
}

Window::Window(const char *title, SDL_WindowFlags flags, uint32_t width, uint32_t height)
{
  InitSDL();
  windowTitle = title;
  windowFlags = flags;
  state.width = width;
  state.height = height;
}

Window::~Window()
{
  CleanSDL();
}

void Window::LoopHandler(glm::vec4 backgroundColor)
{
  loopState = LoopState::RUNNING;
  bool demoMenu = true;

  VulkanRenderer renderer(state.window, state.width, state.height);
  if(!renderer.InitVulkan())
  {
    SDL_Log("Counld't init vulkan");
    return;
  }

  while(loopState != LoopState::QUIT)
  {
    SDL_Event event { 0 };
    while (SDL_PollEvent(&event))
    {
      // Poll events for ImGui and SDL3
      //ImGui_ImplSDL3_ProcessEvent(&event);
      EventHandler(event);
    }

    renderer.width = state.width;
    renderer.height = state.height;
    renderer.Render(backgroundColor);
  }

  //CleanImGuiMenu(&ImGui_ImplSDLRenderer3_Shutdown);
  renderer.CleanVulkan();
  CleanSDL();
}

void Window::KeyboardHandler(SDL_Event &event, LoopState &loopState)
{
  switch (event.key.key) 
  {
    case SDLK_Q:
      loopState = LoopState::QUIT;
    break;
  }
}

void Window::MouseHandler(SDL_Event &event)
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

void Window::EventHandler(SDL_Event &event)
{
  switch (event.type)
  {
    case SDL_EVENT_QUIT:
      loopState = LoopState::QUIT;
    break;
    case SDL_EVENT_WINDOW_RESIZED:
      state.width = event.window.data1;
      state.height = event.window.data2;
    break;
    case SDL_EVENT_KEY_UP:
      KeyboardHandler(event, loopState);
    break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      MouseHandler(event);
    break;
  }
}

void Window::CleanSDL()
{
  SDL_DestroyWindow(state.window);
  SDL_Quit();
}
