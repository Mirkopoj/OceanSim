#pragma once

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <memory>

#include "../lve/lve_descriptors.hpp"
#include "../lve/lve_device.hpp"
#include "../lve/lve_game_object.hpp"
#include "../lve/lve_renderer.hpp"
#include "../lve/lve_water.hpp"
#include "../lve/lve_window.hpp"
namespace lve {

class SecondApp {
  public:
   static constexpr int WIDTH = 800;
   static constexpr int HEIGHT = 600;

   SecondApp(size_t);
   ~SecondApp();

   SecondApp(const SecondApp &) = delete;
   SecondApp &operator=(const SecondApp &) = delete;

   void run();

   void loadGameObjects();

  private:
   LveWindow lveWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
   LveDevice lveDevice{lveWindow};
   LveRenderer lveRenderer{lveWindow, lveDevice};

   std::unique_ptr<LveDescriptorPool> globalPool =
       LveDescriptorPool::Builder(lveDevice)
           .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
           .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
           .build();
   std::unique_ptr<LveDescriptorPool> imguiPool =
       LveDescriptorPool::Builder(lveDevice)
           .setMaxSets(400)
           .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 400)
           .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
           .build();
   std::unique_ptr<LveDescriptorPool> computePool =
       LveDescriptorPool::Builder(lveDevice)
           .setMaxSets(300)
           .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 300)
           .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 300)
           .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 600)
           .build();

   std::unique_ptr<LveWater> water = nullptr;

   uint32_t xn = 0;
   uint32_t yn = 0;

   size_t N;

   void fixViewer(LveGameObject &, float);
};
}  // namespace lve
