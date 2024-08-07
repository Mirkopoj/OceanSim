#pragma once

#include <cstdint>
#include <glm/fwd.hpp>
#include <memory>

#include "lve_buffer.hpp"
#include "lve_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace lve {

class LveWater {
  public:
   struct Vertex {
      static std::vector<VkVertexInputBindingDescription>
      getBindingDescriptions();
      static std::vector<VkVertexInputAttributeDescription>
      getAttributeDescriptions();
   };
   LveWater(LveDevice &device, uint32_t x, uint32_t y);
   ~LveWater();

   LveWater(const LveWater &) = delete;
   LveWater &operator=(const LveWater &) = delete;

   static std::unique_ptr<LveWater> createModel(LveDevice &device,
                                                  uint32_t x, uint32_t y);

   void draw(VkCommandBuffer commandBuffer);

  private:
   LveDevice &lveDevice;

   std::unique_ptr<LveBuffer> vertexBuffer;
   uint32_t vertexCount;

   bool hasIndexBuffer = false;
   std::unique_ptr<LveBuffer> indexBuffer;
   uint32_t indexCount;
};

}  // namespace lve
