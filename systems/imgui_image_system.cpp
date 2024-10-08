#include <vulkan/vulkan_core.h>

#include "gui_system.hpp"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "lve/lve_device.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// Helper function to find Vulkan memory type bits. See
// ImGui_ImplVulkan_MemoryType() in imgui_impl_vulkan.cpp
uint32_t findMemoryType(uint32_t type_filter,
                        VkMemoryPropertyFlags properties,
                        lve::LveDevice& device) {
   VkPhysicalDeviceMemoryProperties mem_properties;
   vkGetPhysicalDeviceMemoryProperties(device.physical_device(),
                                       &mem_properties);

   for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
      if ((type_filter & (1 << i)) &&
          (mem_properties.memoryTypes[i].propertyFlags & properties) ==
              properties)
         return i;

   return 0xFFFFFFFF;  // Unable to find memoryType
}

// Helper function to load an image with common settings and return a
// MyTextureData with a VkDescriptorSet as a sort of Vulkan pointer
bool LoadTextureFromFile(const char* filename, MyTextureData* tex_data,
                         lve::LveDevice& device) {
   // Specifying 4 channels forces stb to load the image in RGBA which is
   // an easy format for Vulkan
   tex_data->Channels = 4;
   unsigned char* image_data =
       stbi_load(filename, &tex_data->Width, &tex_data->Height, 0,
                 tex_data->Channels);

   if (image_data == NULL) return false;

   // Calculate allocation size (in number of bytes)
   size_t image_size =
       tex_data->Width * tex_data->Height * tex_data->Channels;

   VkResult err;

   // Upload to Buffer:
   {
      void* map = NULL;
      err = vkMapMemory(device.device(), tex_data->UploadBufferMemory, 0,
                        image_size, 0, &map);
      check_vk_result(err);
      memcpy(map, image_data, image_size);
      VkMappedMemoryRange range[1] = {};
      range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      range[0].memory = tex_data->UploadBufferMemory;
      range[0].size = image_size;
      err = vkFlushMappedMemoryRanges(device.device(), 1, range);
      check_vk_result(err);
      vkUnmapMemory(device.device(), tex_data->UploadBufferMemory);
   }

   // Release image memory using stb
   stbi_image_free(image_data);

   return true;
}

MyTextureData::MyTextureData(size_t width, size_t height, size_t channels,
                             lve::LveDevice& device, VkFormat format)
    : Width(width), Height(height), Channels(channels), device(device) {
   // Calculate allocation size (in number of bytes)
   size_t image_size = this->Width * this->Height * this->Channels * 2;

   VkResult err;

   // Create the Vulkan image.
   {
      VkImageCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      info.imageType = VK_IMAGE_TYPE_2D;
      info.format = format;
      info.extent.width = this->Width;
      info.extent.height = this->Height;
      info.extent.depth = 1;
      info.mipLevels = 1;
      info.arrayLayers = 1;
      info.samples = VK_SAMPLE_COUNT_1_BIT;
      info.tiling = VK_IMAGE_TILING_OPTIMAL;
      info.usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                   VK_IMAGE_USAGE_STORAGE_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      err = vkCreateImage(device.device(), &info, nullptr, &this->Image);
      check_vk_result(err);
      VkMemoryRequirements req;
      vkGetImageMemoryRequirements(device.device(), this->Image, &req);
      VkMemoryAllocateInfo alloc_info = {};
      alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      alloc_info.allocationSize = req.size;
      alloc_info.memoryTypeIndex = findMemoryType(
          req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device);
      err = vkAllocateMemory(device.device(), &alloc_info, nullptr,
                             &this->ImageMemory);
      check_vk_result(err);
      err = vkBindImageMemory(device.device(), this->Image,
                              this->ImageMemory, 0);
      check_vk_result(err);
   }

   // Create the Image View
   {
      VkImageViewCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      info.image = this->Image;
      info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      info.format = format;
      info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      info.subresourceRange.levelCount = 1;
      info.subresourceRange.layerCount = 1;
      err = vkCreateImageView(device.device(), &info, nullptr,
                              &this->ImageView);
      check_vk_result(err);
   }

   // Create Sampler
   {
      VkSamplerCreateInfo sampler_info{};
      sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
      sampler_info.magFilter = VK_FILTER_LINEAR;
      sampler_info.minFilter = VK_FILTER_LINEAR;
      sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      sampler_info.minLod = -1000;
      sampler_info.maxLod = 1000;
      sampler_info.maxAnisotropy = 1.0f;
      err = vkCreateSampler(device.device(), &sampler_info, nullptr,
                            &this->Sampler);
      check_vk_result(err);
   }

   // Create Descriptor Set using ImGUI's implementation
   this->DS = ImGui_ImplVulkan_AddTexture(this->Sampler, this->ImageView,
                                          VK_IMAGE_LAYOUT_GENERAL);

   // Create Upload Buffer
   {
      VkBufferCreateInfo buffer_info = {};
      buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_info.size = image_size;
      buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      err = vkCreateBuffer(device.device(), &buffer_info, nullptr,
                           &this->UploadBuffer);
      check_vk_result(err);
      VkMemoryRequirements req;
      vkGetBufferMemoryRequirements(device.device(), this->UploadBuffer,
                                    &req);
      VkMemoryAllocateInfo alloc_info = {};
      alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      alloc_info.allocationSize = req.size;
      alloc_info.memoryTypeIndex = findMemoryType(
          req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device);
      err = vkAllocateMemory(device.device(), &alloc_info, nullptr,
                             &this->UploadBufferMemory);
      check_vk_result(err);
      err = vkBindBufferMemory(device.device(), this->UploadBuffer,
                               this->UploadBufferMemory, 0);
      check_vk_result(err);
   }

   // Create a command buffer that will perform following steps when hit in
   // the command queue.
   // TODO: this works in the example, but may need input if this is an
   // acceptable way to access the pool/create the command buffer.
   VkCommandPool command_pool = device.getCommandPool();
   VkCommandBuffer command_buffer;
   {
      VkCommandBufferAllocateInfo alloc_info{};
      alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      alloc_info.commandPool = command_pool;
      alloc_info.commandBufferCount = 1;

      err = vkAllocateCommandBuffers(device.device(), &alloc_info,
                                     &command_buffer);
      check_vk_result(err);

      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      err = vkBeginCommandBuffer(command_buffer, &begin_info);
      check_vk_result(err);
   }

   // Copy to Image
   {
      VkImageMemoryBarrier copy_barrier[1] = {};
      copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      copy_barrier[0].image = this->Image;
      copy_barrier[0].subresourceRange.aspectMask =
          VK_IMAGE_ASPECT_COLOR_BIT;
      copy_barrier[0].subresourceRange.levelCount = 1;
      copy_barrier[0].subresourceRange.layerCount = 1;
      vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0,
                           NULL, 1, copy_barrier);

      VkBufferImageCopy region = {};
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.layerCount = 1;
      region.imageExtent.width = this->Width;
      region.imageExtent.height = this->Height;
      region.imageExtent.depth = 1;
      vkCmdCopyBufferToImage(
          command_buffer, this->UploadBuffer, this->Image,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

      VkImageMemoryBarrier use_barrier[1] = {};
      use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      use_barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
      use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      use_barrier[0].image = this->Image;
      use_barrier[0].subresourceRange.aspectMask =
          VK_IMAGE_ASPECT_COLOR_BIT;
      use_barrier[0].subresourceRange.levelCount = 1;
      use_barrier[0].subresourceRange.layerCount = 1;
      vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                           NULL, 0, NULL, 1, use_barrier);
   }

   // End command buffer
   {
      VkSubmitInfo end_info = {};
      end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      end_info.commandBufferCount = 1;
      end_info.pCommandBuffers = &command_buffer;
      err = vkEndCommandBuffer(command_buffer);
      check_vk_result(err);
      err = vkQueueSubmit(device.graphicsQueue(), 1, &end_info,
                          VK_NULL_HANDLE);
      check_vk_result(err);
      err = vkDeviceWaitIdle(device.device());
      check_vk_result(err);
   }
}

// Helper function to cleanup an image loaded with LoadTextureFromFile
MyTextureData::~MyTextureData() {
   vkFreeMemory(this->device.device(), this->UploadBufferMemory, nullptr);
   vkDestroyBuffer(this->device.device(), this->UploadBuffer, nullptr);
   vkDestroySampler(this->device.device(), this->Sampler, nullptr);
   vkDestroyImageView(this->device.device(), this->ImageView, nullptr);
   vkDestroyImage(this->device.device(), this->Image, nullptr);
   vkFreeMemory(this->device.device(), this->ImageMemory, nullptr);
   ImGui_ImplVulkan_RemoveTexture(this->DS);
}
