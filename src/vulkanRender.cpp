#include "headers/vulkanRender.hpp"

#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallBack(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallBackData,
      void *pUserData)
{
  if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    std::cerr << "Validation Layer: " << pCallBackData->pMessage << std::endl;
  }
  return VK_FALSE;
}

VulkanRenderer::VulkanRenderer(SDL_Window *win)
{
  window = win;
}

bool VulkanRenderer::InitVulkan()
{
  if(!CreateVulkanInstance())
  {
    SDL_Log("Error: Couldn't create a vulkan instance");
    return false;
  }
  if(!CreateSurface())
  {
    SDL_Log("Error: Couldn't create window surface");
    return false;
  }
  if(physicalDevice = FindPhysicalDevice(); !physicalDevice)
  {
    SDL_Log("Unable to find appropriate physical device");
    return false;
  }

  return true;
}

bool VulkanRenderer::CreateVulkanInstance()
{
  if(volkInitialize() != VK_SUCCESS)
  {
    return false;
  }
  
  // Create Vulkan app instance
  VkApplicationInfo appInfo
  {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "My first Vulkan Render",
    .apiVersion = VK_API_VERSION_1_4,
  };

  //Asking SDL what extension it needs to create a vulkan surface
  uint32_t instExtCount = 0;
  const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&instExtCount);
  std::vector<const char *> requestedExtensions{VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
  for(int i = 0; i < instExtCount; i++)
  {
    requestedExtensions.push_back(extensions[i]);
  }

  // Enabling validation layers for error checking and reporting
  std::vector<const char *> requestedLayers{"VK_LAYER_KHRONOS_validation"};

  // Debug struct
  VkDebugUtilsMessengerCreateInfoEXT debugInfo
  {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debugCallBack

  };

  // Create Vulkan Instance
  VkInstanceCreateInfo instCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = &debugInfo,
    .pApplicationInfo = &appInfo,
    .enabledLayerCount = static_cast<uint32_t>(requestedLayers.size()),
    .ppEnabledLayerNames = requestedLayers.data(),
    .enabledExtensionCount = static_cast<uint32_t>(requestedExtensions.size()),
    .ppEnabledExtensionNames = requestedExtensions.data()
  };

  if(vkCreateInstance(&instCreateInfo, nullptr, &vulkanInstance) != VK_SUCCESS)
  {
    return false;
  }

  volkLoadInstance(vulkanInstance);
  return true;
}

bool VulkanRenderer::CreateSurface()
{
  if(!SDL_Vulkan_CreateSurface(window, vulkanInstance, nullptr, &surface))
  {
    return false;
  }
  return true;
}

VkPhysicalDevice VulkanRenderer::FindPhysicalDevice()
{
  // Count physical devices
  uint32_t physDeviceCount = 0;
  vkEnumeratePhysicalDevices(vulkanInstance, &physDeviceCount, nullptr);
  std::vector<VkPhysicalDevice> physicalDevices(physDeviceCount);
  vkEnumeratePhysicalDevices(vulkanInstance, &physDeviceCount, physicalDevices.data());

  VkPhysicalDevice physicalDevice = nullptr;
  if(physDeviceCount)
  {
    // Default to first GPU
    physicalDevice = physicalDevices[0];
    // look through list to see if exist
    for(auto &pDev : physicalDevices)
    {
      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(pDev, &props);
      if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      {
        physicalDevice = pDev;
        break;
      }
    }
  }

  return physicalDevice;
}

void VulkanRenderer::CleanVulkan()
{
  if(vulkanInstance)
  {
    vkDestroyInstance(vulkanInstance, nullptr);
  }
  volkFinalize();
}
