#pragma once

#define VK_NO_PROTOTYPES
#include <SDL3/SDL_vulkan.h>
#include <string>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <string>
#include <shaderc/shaderc.hpp>

class VulkanRenderer
{
  // debug function
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallBack(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallBackData,
      void *pUserData);

  VulkanRenderer(SDL_Window *win);

  // SDL
  SDL_Window *window;

  // vulkan core
  VkInstance vulkanInstance = nullptr;
  VkSurfaceKHR surface = nullptr;
  VkPhysicalDevice physicalDevice = nullptr;

  bool InitVulkan();
  bool CreateVulkanInstance();
  bool CreateSurface();
  VkPhysicalDevice FindPhysicalDevice();
  void CleanVulkan();
};
