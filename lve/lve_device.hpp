#pragma once

#include "lve_window.hpp"

// std lib headers
#include <vector>

namespace lve {

struct SwapChainSupportDetails {
   VkSurfaceCapabilitiesKHR capabilities;
   std::vector<VkSurfaceFormatKHR> formats;
   std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
   uint32_t graphicsFamily;
   uint32_t presentFamily;
   uint32_t computeFamily;
   bool graphicsFamilyHasValue = false;
   bool presentFamilyHasValue = false;
   bool computeFamilyHasValue = false;
   bool isComplete() {
      return graphicsFamilyHasValue && presentFamilyHasValue &&
             computeFamilyHasValue;
   }
};

class LveDevice {
  public:
#ifdef NDEBUG
   const bool enableValidationLayers = false;
#else
   const bool enableValidationLayers = true;
#endif

   LveDevice(LveWindow &window);
   ~LveDevice();

   // Not copyable or movable
   LveDevice(const LveDevice &) = delete;
   LveDevice &operator=(const LveDevice &) = delete;
   LveDevice(LveDevice &&) = delete;
   LveDevice &operator=(LveDevice &&) = delete;

   VkCommandPool getCommandPool() {
      return commandPool;
   }
   VkDevice device() {
      return device_;
   }
   VkSurfaceKHR surface() {
      return surface_;
   }
   VkQueue graphicsQueue() {
      return graphicsQueue_;
   }
   VkQueue presentQueue() {
      return presentQueue_;
   }
   VkQueue computeQueue() {
      return computeQueue_;
   }

   VkPhysicalDevice physical_device() {
      return physicalDevice;
   }
   VkInstance get_instance() {
      return instance;
   }

   SwapChainSupportDetails getSwapChainSupport() {
      return querySwapChainSupport(physicalDevice);
   }
   uint32_t findMemoryType(uint32_t typeFilter,
                           VkMemoryPropertyFlags properties);
   QueueFamilyIndices findPhysicalQueueFamilies() {
      return findQueueFamilies(physicalDevice);
   }
   VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                                VkImageTiling tiling,
                                VkFormatFeatureFlags features);

   // Buffer Helper Functions
   void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkBuffer &buffer,
                     VkDeviceMemory &bufferMemory);
   VkCommandBuffer beginSingleTimeCommands();
   void endSingleTimeCommands(VkCommandBuffer commandBuffer);
   void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                   VkDeviceSize size);
   void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                          uint32_t height, uint32_t layerCount);

   void createImageWithInfo(const VkImageCreateInfo &imageInfo,
                            VkMemoryPropertyFlags properties,
                            VkImage &image, VkDeviceMemory &imageMemory);

   VkPhysicalDeviceProperties properties;

   VkCommandBuffer beginCommandBuffer();
   void endCommandBuffer(VkCommandBuffer commandBuffer);

  private:
   void createInstance();
   void setupDebugMessenger();
   void createSurface();
   void pickPhysicalDevice();
   void createLogicalDevice();
   void createCommandPool();

   // helper functions
   bool isDeviceSuitable(VkPhysicalDevice device);
   std::vector<const char *> getRequiredExtensions();
   bool checkValidationLayerSupport();
   QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
   void populateDebugMessengerCreateInfo(
       VkDebugUtilsMessengerCreateInfoEXT &createInfo);
   void hasGflwRequiredInstanceExtensions();
   bool checkDeviceExtensionSupport(VkPhysicalDevice device);
   SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

   VkInstance instance;
   VkDebugUtilsMessengerEXT debugMessenger;
   VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
   LveWindow &window;
   VkCommandPool commandPool;

   VkDevice device_;
   VkSurfaceKHR surface_;
   VkQueue graphicsQueue_;
   VkQueue presentQueue_;
   VkQueue computeQueue_;

   const std::vector<const char *> validationLayers = {
       "VK_LAYER_KHRONOS_validation"};
   const std::vector<const char *> deviceExtensions = {
       VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

}  // namespace lve
