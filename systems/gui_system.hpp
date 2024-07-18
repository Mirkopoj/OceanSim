#pragma once

#include <cstring>
#include <cwchar>

#include "../lve/lve_device.hpp"
#include "../lve/lve_renderer.hpp"
#include "../movement_controllers/terrain_movement_controller.hpp"

typedef struct {
   glm::float32 scale;
   glm::float32 angle;
   glm::float32 spreadBlend;
   glm::float32 swell;
   glm::float32 alpha;
   glm::float32 peakOmega;
   glm::float32 gamma;
   glm::float32 shortWavesFade;
} SpectrumParameters;

struct MyTextureData {
   VkDescriptorSet
       DS;  // Descriptor set: this is what you'll pass to Image()
   int Width;
   int Height;
   int Channels;

   // Need to keep track of these to properly cleanup
   VkImageView ImageView;
   VkImage Image;
   VkDeviceMemory ImageMemory;
   VkSampler Sampler;
   VkBuffer UploadBuffer;
   VkDeviceMemory UploadBufferMemory;
   lve::LveDevice &device;

   MyTextureData(size_t width, size_t height, size_t channels,
                 lve::LveDevice &device, VkFormat format);
   ~MyTextureData();
};

MyTextureData new_tex(size_t width, size_t height, size_t channels,
                      lve::LveDevice &device);

const char *vk_result_to_c_string(VkResult result);

static void check_vk_result(VkResult err) {
   if (err == 0) return;
   fprintf(stderr, "[vulkan] Error: VkResult = %s\n",
           vk_result_to_c_string(err));
   if (err < 0) abort();
}

class ImGuiGui {
  public:
   ImGuiGui(GLFWwindow *window, lve::LveDevice &lveDevice,
            lve::LveRenderer &lveRenderer, VkDescriptorPool imguiPool);
   ImGuiGui(ImGuiGui &&) = delete;
   ImGuiGui(const ImGuiGui &) = delete;
   ImGuiGui &operator=(ImGuiGui &&) = delete;
   ImGuiGui &operator=(const ImGuiGui &) = delete;
   ~ImGuiGui();

   void new_frame();
   void update(lve::TerrainMovementController &cameraControler,
               bool &caminata, size_t &pipeline, glm::vec3 coord,
               float frameTime, MyTextureData *img[],
               SpectrumParameters &params);
   void render(VkCommandBuffer command_buffer);
};
