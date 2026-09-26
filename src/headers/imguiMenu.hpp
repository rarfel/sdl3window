#pragma once
#include "../../vendor/imgui/imgui.h"
#include "../../vendor/imgui/imgui_impl_sdl3.h"
#include "../../vendor/imgui/imgui_impl_sdlrenderer3.h"

ImGuiIO* InitImGuiMenu(bool (implRenderer)(SDL_Window*), SDL_Window *window, SDL_Renderer *renderer);
ImGuiIO* InitImGuiMenu(bool (implRenderer)(SDL_Window*,SDL_Renderer*), SDL_Window *window, SDL_Renderer *renderer);
void StartImGuiFrame(void (implRenderer)());
void CleanImGuiMenu(void(implRenderer()));

template<typename T>
void RenderImGuiFrame(void(*implRenderGui)(ImDrawData*, T), T renderCtx)
{
    ImGui::Render();
    implRenderGui(ImGui::GetDrawData(), renderCtx);
}
