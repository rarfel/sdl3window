#pragma once

#define VK_NO_PROTOTYPES
#include <SDL3/SDL_vulkan.h>
#include <string>
#include <iostream>
#include <vulkan/vulkan.h>
#include <array>
#include <shaderc/shaderc.hpp>

struct VmaAllocator_T;
typedef struct VmaAllocator_T* VmaAllocator;
struct VmaAllocation_T;
typedef struct VmaAllocation_T* VmaAllocation;

struct FrameResources
{
	VkCommandPool commandPool = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	VkSemaphore imageAcquiredSemaphore = nullptr;
};

class VulkanRenderer
{
  // debug function
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallBack(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallBackData,
      void *pUserData);

  constexpr static uint32_t VULKAN_VERSION{VK_API_VERSION_1_4};
  constexpr static uint32_t MaxFramesInFlight{ 2 };
  constexpr static VkFormat swapchainFormat{ VK_FORMAT_B8G8R8A8_SRGB };
  constexpr static VkFormat depthFormat{ VK_FORMAT_D32_SFLOAT };

  uint64_t frameIndex = 0;
  uint64_t nextSignalValue = MaxFramesInFlight + 1;

  // SDL
  SDL_Window *window;
  uint32_t width;
  uint32_t height;

  // vulkan core
  VkInstance vulkanInstance = nullptr;
  VkSurfaceKHR surface = nullptr;
  VkPhysicalDevice physicalDevice = nullptr;
  VkDevice device = nullptr;
  VmaAllocator vmaAllocator = nullptr;

  // queue
  uint32_t gfxQueueFamIdx = UINT32_MAX;
  VkQueue gfxQueue = nullptr;

  // swapchain
  VkSwapchainKHR swapchain = nullptr;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	bool requireSwapchainRecreate = false;
  uint32_t swapchainWidth = 0;
	uint32_t swapchainHeight = 0;

  VkImage depthImage = nullptr;
	VkImageView depthImageView = nullptr;
	VmaAllocation depthImageAllocation = nullptr;

  // shader
	VkShaderModule vertShader = nullptr;
	VkShaderModule fragShader = nullptr;

  // graphics pipeline
	VkPipelineLayout pipelineLayout = nullptr;
	VkPipeline pipeline = nullptr;

  // frame and synchronization resources
	VkSemaphore timelineSemaphore = nullptr;
	std::array<FrameResources, MaxFramesInFlight> frameResources;

  public:
  VulkanRenderer(SDL_Window *win, uint32_t w, uint32_t h);
  bool InitVulkan();

  bool CreateVulkanInstance();
  bool CreateSurface();
  VkPhysicalDevice FindPhysicalDevice();
  bool FindGraphicsQueue();
  bool CreateDevice(VkPhysicalDevice physicalDevice);
  bool InitializeVMA();
  bool CreateSwapchain(uint32_t width, uint32_t height);
  void DestroySwapchain();
  VkShaderModule CreateShaderModule(const std::string &fileName, shaderc_shader_kind kind);
  bool CreateShaders();
  VkPipeline CreateGraphicPipeline();
  bool CreateSyncResources();
  bool CreateCommandBuffers();

  void Render();

  void CleanVulkan();
};
