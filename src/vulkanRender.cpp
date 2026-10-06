#include "headers/vulkanRender.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdint>
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

VulkanRenderer::VulkanRenderer(SDL_Window *win, int w, int h)
{
  window = win;
  width = w;
  height = h;
}

VulkanRenderer::~VulkanRenderer()
{
  CleanVulkan();
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

  if(!FindGraphicsQueue())
  {
    SDL_Log("Unable to find compatible graphics queue");
    return false;
  }

  if(!CreateDevice(physicalDevice))
  {
    SDL_Log("Couldn't create the logical GPU device");
    return false;
  }

  if(!InitializeVMA())
  {
    SDL_Log("Unable to create Vulkan Memory Allocator");
    return false;
  }

  if(!CreateSwapchain(width, height))
  {
    SDL_Log("Couldn't create swapchain");
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
    .apiVersion = VULKAN_VERSION,
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

  // ensure the desired swapchain format is supported
  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
  std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());

  bool formatSupported = false;
  for(const VkSurfaceFormatKHR &surfFormat : surfaceFormats)
  {
    if(surfFormat.format == swapchainFormat)
    {
      formatSupported = true;
      break;
    }
  }

  if(!formatSupported)
  {
    SDL_Log("Requested Swapchain format not supported by the surface");
    return nullptr;
  }

  return physicalDevice;
}

bool VulkanRenderer::FindGraphicsQueue()
{
  // grab all queue families
  uint32_t queueFamCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamCount, nullptr);
  std::vector<VkQueueFamilyProperties2> queueFamProps(queueFamCount,
      {.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
  vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamCount, queueFamProps.data());

  for(int currentFamIdx = 0; currentFamIdx < queueFamProps.size(); currentFamIdx++)
  {
    // ensuring it has presentation support
    VkBool32 hasPresentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, currentFamIdx, surface, &hasPresentSupport);

    const auto &props = queueFamProps[currentFamIdx];
    // ensuring it is Graphics queue with presentation support
    if(props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && hasPresentSupport)
    {
      gfxQueueFamIdx = currentFamIdx;
      return true;
    }
  }
  return false;
}

bool VulkanRenderer::CreateDevice(VkPhysicalDevice physicalDevice)
{
  // query supported features
  VkPhysicalDeviceVulkan14Features supportedFeatures14{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, .pNext = nullptr};
  VkPhysicalDeviceVulkan13Features supportedFeatures13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &supportedFeatures14};
  VkPhysicalDeviceVulkan12Features supportedFeatures12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &supportedFeatures13};
  VkPhysicalDeviceFeatures2 supportedFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &supportedFeatures12};

  vkGetPhysicalDeviceFeatures2(physicalDevice, &supportedFeatures);
  
  // check if render has support for what it needs
  if(!supportedFeatures13.dynamicRendering || !supportedFeatures13.synchronization2 || !supportedFeatures12.timelineSemaphore)
  {
    SDL_Log("Physical device doesn't meet the feature requirements");
    return false;
  }

  // produce a separate features struct chain for the device creation
  // manually enabling ONLY features the render will need
  VkPhysicalDeviceVulkan14Features features14
  {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
    .pNext = nullptr,
  };
  VkPhysicalDeviceVulkan13Features features13
  {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, 
    .pNext = &features14,
    .synchronization2 = VK_TRUE,
    .dynamicRendering = VK_TRUE,
  };
  VkPhysicalDeviceVulkan12Features features12
  {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, 
    .pNext = &features13,
    .timelineSemaphore = VK_TRUE,
  };
  VkPhysicalDeviceFeatures2 features
  {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &features12,
  };

  // request the queues the render will use
  std::vector<float> queuePriorities{1.0};
  VkDeviceQueueCreateInfo gfxQueueInfo
  {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = gfxQueueFamIdx,
    .queueCount = 1,
    .pQueuePriorities = queuePriorities.data()
  };

  // device specific extensions
  const std::vector<const char *> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkDeviceCreateInfo devCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &features,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &gfxQueueInfo,
    .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
    .ppEnabledExtensionNames = deviceExtensions.data(),
    .pEnabledFeatures = nullptr // features struct is set in pNext
  };

  if(vkCreateDevice(physicalDevice, &devCreateInfo, nullptr, &device) != VK_SUCCESS)
  {
    return false;
  }

  // grab VkQueue object
  vkGetDeviceQueue(device, gfxQueueFamIdx, 0, &gfxQueue);
  if(!gfxQueue)
  {
    SDL_Log("Couldn't get graphics queue");
    return false;
  }
  return true;
}

bool VulkanRenderer::InitializeVMA()
{
  VmaVulkanFunctions vmaFuncInfo{};
  VmaAllocatorCreateInfo vmaAllocInfo
  {
    .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    .physicalDevice = physicalDevice,
    .device = device,
    .pVulkanFunctions = &vmaFuncInfo,
    .instance = vulkanInstance,
    .vulkanApiVersion = VULKAN_VERSION
  };

  // vma can import from volk
  vmaImportVulkanFunctionsFromVolk(&vmaAllocInfo, &vmaFuncInfo);
  
  if(vmaCreateAllocator(&vmaAllocInfo, &vmaAllocator) != VK_SUCCESS)
  {
    return false;
  }

  return true;
}

bool VulkanRenderer::CreateSwapchain(uint32_t width, uint32_t height)
{
  // track swapchain size separate from window size
  swapchainWidth = width;
  swapchainHeight = height;

  // requesting appropriate number of images
  VkSurfaceCapabilitiesKHR surfaceCaps{};
  if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps) != VK_SUCCESS)
  {
    SDL_Log("Couldn't get surface capabilities");
    return false;
  }

  uint32_t requestedImageCount = std::max(2u, surfaceCaps.minImageCount);
  if(surfaceCaps.maxImageCount > 0)
  {
    requestedImageCount = std::min(requestedImageCount, surfaceCaps.maxImageCount);
  }

  VkSwapchainCreateInfoKHR swapchainCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface,
    .minImageCount = requestedImageCount,
    .imageFormat = swapchainFormat,
    .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
    .imageExtent{.width = swapchainWidth, .height = swapchainHeight},
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = surfaceCaps.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
  };

  if(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS)
  {
    SDL_Log("ERROR creating swapchain");
    return false;
  }

  // asking for the swapchain images
  uint32_t imageCount = 0;
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
  swapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
  swapchainImageViews.resize(imageCount);

  // create swapchain image views
  for(size_t i = 0; i < swapchainImages.size(); i++)
  {
    VkImageViewCreateInfo imgViewInfo
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = swapchainImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = swapchainFormat,
      .subresourceRange
      {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    if(vkCreateImageView(device, &imgViewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
    {
      SDL_Log("ERROR creating swapchain image view");
      return false;
    }
  }

  // semaphor used to signal the render completion
  renderCompleteSemaphores.resize(swapchainImages.size());
  for(VkSemaphore &semaphor : renderCompleteSemaphores)
  {
    VkSemaphoreCreateInfo semaphorInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if(vkCreateSemaphore(device, &semaphorInfo, nullptr, &semaphor) != VK_SUCCESS)
    {
      SDL_Log("ERROR creating the render-complete semaphor");
      return false;
    }
  }

  // create zBuffer
  VkImageCreateInfo depthCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = depthFormat,
    .extent{.width = swapchainWidth, .height = swapchainHeight, .depth = 1},
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
  };

  VmaAllocationCreateInfo allocInfo
  {
    .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO,
  };
  if(vmaCreateImage(vmaAllocator, &depthCreateInfo, &allocInfo, &depthImage, &depthImageAllocation, nullptr) != VK_SUCCESS)
  {
    SDL_Log("ERROR allocating depth image");
    return false;
  }

  VkImageViewCreateInfo depthImgViewInfo
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = depthImage,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = depthFormat,
    .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1},
  };
  if(vkCreateImageView(device, &depthImgViewInfo, nullptr, &depthImageView) != VK_SUCCESS)
  {
    SDL_Log("ERROR creating depth image view");
    return false;
  }

  return true;
}

void VulkanRenderer::CleanVulkan()
{
  // vma
  if(vmaAllocator)
  {
    vmaDestroyAllocator(vmaAllocator);
  }

  //vulkan

  if(surface)
  {
    vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
  }

  if(device)
  {
    vkDestroyDevice(device, nullptr);
  }

  if(vulkanInstance)
  {
    vkDestroyInstance(vulkanInstance, nullptr);
  }
  volkFinalize();
}
