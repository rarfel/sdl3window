#include "headers/vulkanRender.hpp"
#include "headers/fileReader.hpp"

#include <SDL3/SDL.h>
#include <cstdint>
#include <shaderc/shaderc.h>
#include <shaderc/status.h>
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

VulkanRenderer::VulkanRenderer(SDL_Window *win, uint32_t w, uint32_t h)
{
  window = win;
  width = w;
  height = h;
}

bool VulkanRenderer::InitVulkan()
{
  if(!CreateVulkanInstance())
  {
    std::print("Error: Couldn't create a vulkan instance");
    return false;
  }

  if(!CreateSurface())
  {
    std::print("Error: Couldn't create window surface");
    return false;
  }

  if(physicalDevice = FindPhysicalDevice(); !physicalDevice)
  {
    std::print("Unable to find appropriate physical device");
    return false;
  }

  if(!FindGraphicsQueue())
  {
    std::print("Unable to find compatible graphics queue");
    return false;
  }

  if(!CreateDevice(physicalDevice))
  {
    std::print("Couldn't create the logical GPU device");
    return false;
  }

  if(!InitializeVMA())
  {
    std::print("Unable to create Vulkan Memory Allocator");
    return false;
  }

  if(!CreateSwapchain(width, height))
  {
    std::print("Couldn't create swapchain");
    return false;
  }

  if(!CreateShaders())
  {
    std::print("Couldn't create shaders modules");
    return false;
  }

  if(pipeline = CreateGraphicPipeline(); !pipeline)
  {
    std::print("Unable to initialize graphics pipeline");
    return false;
  }

  if(!CreateSyncResources())
  {
    std::print("Couldn't create the sync related resources");
    return false;
  }

  if(!CreateCommandBuffers())
  {
    std::print("Couldn't create command buffers");
    return false;
  }

  return true;
}

void VulkanRenderer::Render(glm::vec4 backgroundColor)
{
  // first check if swapchain is valid
  if(requireSwapchainRecreate)
  {
    vkDeviceWaitIdle(device);
    DestroySwapchain();
    CreateSwapchain(width, height);
    requireSwapchainRecreate = false;
  }

  const uint32_t frameResIndex = frameIndex++ % MaxFramesInFlight;
  const uint64_t signalValue = nextSignalValue++;
  const uint64_t waitValue = signalValue - MaxFramesInFlight;

  VkSemaphoreWaitInfo waitInfo
  {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
    .semaphoreCount = 1,
    .pSemaphores = &timelineSemaphore,
    .pValues = &waitValue
  };
  vkWaitSemaphores(device, &waitInfo, UINT64_MAX);

  // starting record commands
  FrameResources &res = frameResources[frameResIndex];
  vkResetCommandPool(device, res.commandPool, 0);

  // get the resources for this frame
  VkSemaphore imageAcquireSemaphore = frameResources[frameResIndex].imageAcquiredSemaphore;

  uint32_t imageIndex = 0;
  VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquireSemaphore, VK_NULL_HANDLE, &imageIndex);

  // handle resize and out-of-date images, may need swapchain recreate
  if(acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
  {
    requireSwapchainRecreate = true;
    return;
  }
  else if(acquireResult == VK_SUBOPTIMAL_KHR)
  {
    // can render this frame, but recreate the next
    requireSwapchainRecreate = true;
  }

  // begin recording commands
  VkCommandBufferBeginInfo cmdBeginInfo
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  vkBeginCommandBuffer(res.commandBuffer, &cmdBeginInfo);

  // transition the color and depth images
  std::vector<VkImageMemoryBarrier2> layoutBarriers
  {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .image = swapchainImages[imageIndex],
      .subresourceRange
      {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      }
    },
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | 
                      VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
      .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .image = depthImage,
      .subresourceRange
      {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      }
    },
  };
  VkDependencyInfo depInfo
  {
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = static_cast<uint32_t>(layoutBarriers.size()),
      .pImageMemoryBarriers = layoutBarriers.data()
  };
  vkCmdPipelineBarrier2(res.commandBuffer, &depInfo);

  // setup the attachments (color and depth) and begin rendering (dynamic)
  VkRenderingAttachmentInfo colorAttachInfo
  {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = swapchainImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, //clear the image
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // keep data for presentation
    .clearValue{.color{backgroundColor.r,backgroundColor.g,backgroundColor.b,backgroundColor.a}}
  };
  VkRenderingAttachmentInfo depthAttachInfo
  {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = depthImageView,
    .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, //clear the depth data
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, // discard after rendering
    .clearValue{.depthStencil{1.0,0}}
  };
  VkRenderingInfo renderingInfo
  {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea
    {
      .offset{.x = 0, .y = 0},
      .extent{.width = swapchainWidth, .height = swapchainHeight}
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachInfo,
    .pDepthAttachment = &depthAttachInfo,
  };

  //begin dynamic rendering
  vkCmdBeginRendering(res.commandBuffer, &renderingInfo);
  {
    // set the viewport and scissor state
    VkViewport viewport
    {
      .x = 0, .y = 0,
      .width = static_cast<float>(swapchainWidth),
      .height = static_cast<float>(swapchainHeight),
    };
    vkCmdSetViewport(res.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor
    {
      .offset{.x = 0, .y = 0},
      .extent{.width = swapchainWidth, .height = swapchainHeight},
    };
    vkCmdSetScissor(res.commandBuffer, 0, 1, &scissor);

    // draw triangle
    vkCmdBindPipeline(res.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDraw(res.commandBuffer, 3, 1, 0, 0);
  }
  // end dynamic rendering
  vkCmdEndRendering(res.commandBuffer);

  // transition the image from color attachment to presentation on screen
  VkImageMemoryBarrier2 presentLayoutBarrier
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_2_NONE, // cache is flushed, layout is transitioned
    .dstAccessMask = 0,
    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    .image = swapchainImages[imageIndex],
    .subresourceRange
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
  };
  VkDependencyInfo presentDepInfo
  {
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &presentLayoutBarrier,
  };
  vkCmdPipelineBarrier2(res.commandBuffer, &presentDepInfo);

  vkEndCommandBuffer(res.commandBuffer);

  // ensure swapchain image is actually vailable to start color output
  VkSemaphoreSubmitInfo imageAcquireWaitInfo
  {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .semaphore = imageAcquireSemaphore,
    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT // wait before drawing to image
  };
  // signal that the image can be presented
  std::vector<VkSemaphoreSubmitInfo> semaphoreSignals
  {
    { // render work completion signal
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = renderCompleteSemaphores[imageIndex],
      .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
    },
    { // entire frame is completed (timeline)
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = timelineSemaphore,
      .value = signalValue,
      .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
    }
  };
  VkCommandBufferSubmitInfo cmdSubmitInfo
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .commandBuffer = res.commandBuffer,
  };
  VkSubmitInfo2 submitInfo
  {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    .waitSemaphoreInfoCount = 1,
    .pWaitSemaphoreInfos = &imageAcquireWaitInfo, // ensure image is ready
    .commandBufferInfoCount = 1,
    .pCommandBufferInfos = &cmdSubmitInfo,
    .signalSemaphoreInfoCount = static_cast<uint32_t>(semaphoreSignals.size()),
    .pSignalSemaphoreInfos = semaphoreSignals.data(),
  };
  vkQueueSubmit2(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE);

  // present the image
  VkPresentInfoKHR presentInfo
  {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &renderCompleteSemaphores[imageIndex], // render work completed semaphore
    .swapchainCount = 1,
    .pSwapchains = &swapchain,
    .pImageIndices = &imageIndex,
    .pResults = nullptr,
  };
  vkQueuePresentKHR(gfxQueue, &presentInfo);
}

bool VulkanRenderer::CreateVulkanInstance()
{
  if(volkInitialize() != VK_SUCCESS)
  {
    std::print("ERROR: couldn't initialize volk");
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
  { requestedExtensions.push_back(extensions[i]); }

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

  if(vkCreateInstance(&instCreateInfo, nullptr, &vulkanInstance)!= VK_SUCCESS)
  { return false; }

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
    std::print("Requested Swapchain format not supported by the surface");
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
    std::print("Physical device doesn't meet the feature requirements");
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
    std::print("Couldn't get graphics queue");
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
    std::print("Couldn't get surface capabilities");
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
    std::print("ERROR creating swapchain");
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
      std::print("ERROR creating swapchain image view");
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
      std::print("ERROR creating the render-complete semaphor");
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
    std::print("ERROR allocating depth image");
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
    std::print("ERROR creating depth image view");
    return false;
  }
  return true;
}

void VulkanRenderer::DestroySwapchain()
{
  for(VkImageView swapchainImgView : swapchainImageViews)
  {
    vkDestroyImageView(device, swapchainImgView, nullptr);
  }
  swapchainImageViews.clear();

  // destroy render-complete semaphores
  for(VkSemaphore &semaphore : renderCompleteSemaphores)
  {
    vkDestroySemaphore(device, semaphore, nullptr);
  }
  renderCompleteSemaphores.clear();

  if(swapchain)
  {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = nullptr;
  }

  // destroy the depth buffer along with the swapchain
  if(depthImageView)
  {
    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(vmaAllocator, depthImage, depthImageAllocation);
    depthImageView = nullptr;
  }
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::string &fileName, shaderc_shader_kind kind)
{
  // read shader file from disk
  const std::string shaderPath = "src/shaders/" + fileName;
  const std::string src = readTextFile(shaderPath);
  if(src.empty())
  {
    std::print("Specified shader file not found: {}", shaderPath);
    return nullptr;
  }

  // compile shaders to SPIR-V
  std::print("Compiling shader: {}\n", shaderPath);
  shaderc::Compiler compiler;
  shaderc::CompileOptions opts;
  opts.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
  opts.SetTargetSpirv(shaderc_spirv_version_1_6);
  opts.SetOptimizationLevel(shaderc_optimization_level_performance);
  shaderc::CompilationResult result = compiler.CompileGlslToSpv(src, kind, fileName.c_str(), opts);

  if(result.GetCompilationStatus() != shaderc_compilation_status_success)
  {
    std::cerr << "Shader Compilation Error: " << result.GetErrorMessage() << std::endl;
    return nullptr;
  }

  const size_t shaderSize = (result.cend() - result.cbegin()) * sizeof(uint32_t);
  // pass SPIR-V to vulkan and create shader-module
  VkShaderModuleCreateInfo moduleCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = shaderSize,
    .pCode = result.cbegin(),
  };

  VkShaderModule shaderModule = nullptr;
  if(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
  {
    std::print("ERROR creating shader module");
    return nullptr;
  }
  return shaderModule;
}

bool VulkanRenderer::CreateShaders()
{
  // creating the shader modules the graphics pipeline will need
  if(vertShader = CreateShaderModule("shader.vert", shaderc_vertex_shader); !vertShader)
  {
    return false;
  }
  if(fragShader = CreateShaderModule("shader.frag", shaderc_fragment_shader); !vertShader)
  {
    return false;
  }
  return true;
}

VkPipeline VulkanRenderer::CreateGraphicPipeline()
{
  // need to define pipeline layout
  VkPipelineLayoutCreateInfo pipelineCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 0,
    .pushConstantRangeCount = 0,
  };
  if(vkCreatePipelineLayout(device, &pipelineCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    std::print("Unable to create pipeline layout");
    return nullptr;
  }

  // configure the shader stages struct
  const char * entryPoint = "main";
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages
  {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertShader,
      .pName = entryPoint,
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragShader,
      .pName = entryPoint,
    },
  };

  // vertex pulling, vertex input details not defined
  VkPipelineVertexInputStateCreateInfo vertInputInfo
  { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, };

  // assembly pulling, draw triangle lists
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };

  // depth/stencil config
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .stencilTestEnable = VK_FALSE,
  };

  // dynamicRendering allows to dynamically(~tada~) change the viewport
  VkPipelineViewportStateCreateInfo viewportInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = nullptr,
    .scissorCount = 1,
    .pScissors = nullptr,
  };

  //rasterizer settings
  VkPipelineRasterizationStateCreateInfo rasterInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .lineWidth = 1.0,
  };

  // no multisampling
  VkPipelineMultisampleStateCreateInfo multisampleInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  // Alpha-bending (disabled), color mask still need to be set
  VkPipelineColorBlendAttachmentState attachState
  {
    .blendEnable = VK_FALSE,
    .colorWriteMask = 
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo blendInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &attachState
  };

  // enable dynamic state
  std::vector<VkDynamicState> dynamicState
  {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicStateInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = static_cast<uint32_t>(dynamicState.size()),
    .pDynamicStates = dynamicState.data(),
  };

  // struct required for dynamic rendering
  VkPipelineRenderingCreateInfo renderInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &swapchainFormat,
    .depthAttachmentFormat = depthFormat,
  };

  // Create graphics pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo
  {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &renderInfo,
    .stageCount = static_cast<uint32_t>(shaderStages.size()),
    .pStages = shaderStages.data(),
    .pVertexInputState = &vertInputInfo,
    .pInputAssemblyState = &inputAssemblyInfo,
    .pViewportState = &viewportInfo,
    .pRasterizationState = &rasterInfo,
    .pMultisampleState = &multisampleInfo,
    .pDepthStencilState = &depthStencilInfo,
    .pColorBlendState = &blendInfo,
    .pDynamicState = &dynamicStateInfo,
    .layout = pipelineLayout,
    .renderPass = VK_NULL_HANDLE,
  };
  
  VkPipeline newPipeline;
  if(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
  {
    std::print("ERROR Creating the pipeline");
    return nullptr;
  }
  return newPipeline;
}

bool VulkanRenderer::CreateSyncResources()
{
  VkSemaphoreTypeCreateInfo semaphoreTypeInfo
  {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
    .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    .initialValue = MaxFramesInFlight,
  };
  VkSemaphoreCreateInfo semaphoreInfo
  {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = &semaphoreTypeInfo,
  };
  if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &timelineSemaphore) != VK_SUCCESS)
  {
    std::print("Unable to create the timeline semaphore");
    return false;
  }

  // per-frame image-acquire semaphores
  for(FrameResources &res : frameResources)
  {
    //create the bynary semaphores
    VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,};
    if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &res.imageAcquiredSemaphore) != VK_SUCCESS)
    {
      std::print("ERROR creating the per-fame image-acquire semaphore");
      return false;
    }
  }
  return true;
}

bool VulkanRenderer::CreateCommandBuffers()
{
  for(FrameResources &res : frameResources)
  {
    // giving each frame its own pool, for faster cmd buffer resets
    VkCommandPoolCreateInfo poolInfo
    {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = gfxQueueFamIdx,
    };
    if(vkCreateCommandPool(device, &poolInfo, nullptr, &res.commandPool) != VK_SUCCESS)
    {
      std::print("Unable to create command buffer pool");
      return false;
    }
    // create the command buffer for this frame
    VkCommandBufferAllocateInfo cmdAllocInfo
    {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = res.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    if(vkAllocateCommandBuffers(device, &cmdAllocInfo, &res.commandBuffer) != VK_SUCCESS)
    {
      std::print("Unable to allocate command buffer");
      return false;
    }
  }
  return true;
}

void VulkanRenderer::CleanVulkan()
{
  // wait in case resources are in use
  vkDeviceWaitIdle(device);

  // frame / sync object cleanup
  if(timelineSemaphore)
  { vkDestroySemaphore(device, timelineSemaphore, nullptr); }

  for(auto &res : frameResources)
  {
    vkDestroySemaphore(device, res.imageAcquiredSemaphore, nullptr);
    vkDestroyCommandPool(device, res.commandPool, nullptr); // destroy buffers also
  }

  // pipeline cleanup
  if(pipelineLayout)
  { vkDestroyPipelineLayout(device, pipelineLayout, nullptr); }

  if(pipeline)
  { vkDestroyPipeline(device, pipeline, nullptr); }

  // shaders cleanup
  if(vertShader)
  { vkDestroyShaderModule(device, vertShader, nullptr); }

  if(fragShader)
  { vkDestroyShaderModule(device, fragShader, nullptr); }

  // cleanup swapchain
  DestroySwapchain();

  // vma
  if(vmaAllocator)
  { vmaDestroyAllocator(vmaAllocator); }

  //vulkan

  if(surface)
  { vkDestroySurfaceKHR(vulkanInstance, surface, nullptr); }

  if(device)
  { vkDestroyDevice(device, nullptr); }

  if(vulkanInstance)
  { vkDestroyInstance(vulkanInstance, nullptr); }

  volkFinalize();
}
