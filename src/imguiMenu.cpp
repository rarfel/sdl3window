#include "headers/imguiMenu.hpp"
#include "SDL3/SDL_render.h"

ImGuiIO* InitImGuiMenu(bool (implRenderer)(SDL_Window*), SDL_Window *window, SDL_Renderer *renderer){
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; //Enable Keyboard
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //Enable Gamepad

  ImGui::StyleColorsDark();
  io.Fonts->AddFontDefault();

  ImGui_ImplSDLRenderer3_Init(renderer);
  implRenderer(window);
  return &io;
}

ImGuiIO* InitImGuiMenu(bool (implRenderer)(SDL_Window*,SDL_Renderer*), SDL_Window *window, SDL_Renderer *renderer){
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; //Enable Keyboard
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //Enable Gamepad

  ImGui::StyleColorsDark();
  io.Fonts->AddFontDefault();

  ImGui_ImplSDLRenderer3_Init(renderer);
  implRenderer(window,renderer);
  return &io;
}

void StartImGuiFrame(void (implRenderer)())
{
    ImGui_ImplSDL3_NewFrame();
    implRenderer();
    ImGui::NewFrame();
}

void CleanImGuiMenu(void(implRenderer()))
{
  ImGui_ImplSDL3_Shutdown();
  implRenderer();
  ImGui::DestroyContext();
}
