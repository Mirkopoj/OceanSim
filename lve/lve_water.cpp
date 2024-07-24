#include "lve_water.hpp"

#include <strings.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <memory>

#define GLM_ENABLE_EXPERIMENTAL
#include <cassert>
#include <cstring>

namespace lve {

LveTerrain::LveTerrain(LveDevice &device, uint32_t x, uint32_t y)
    : lveDevice{device} {
   vertexCount = ((y + (x - 1) * (2 * y - 2)) - 2) * 3;
}

LveTerrain::~LveTerrain() {
}

std::unique_ptr<LveTerrain> LveTerrain::createModel(LveDevice &device,
                                                    uint32_t x,
                                                    uint32_t y) {
   return std::make_unique<LveTerrain>(device, x, y);
}

void LveTerrain::draw(VkCommandBuffer commandBuffer) {
   vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}

std::vector<VkVertexInputBindingDescription>
LveTerrain::Vertex::getBindingDescriptions() {
   std::vector<VkVertexInputBindingDescription> bindingDescriptions(0);
   return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
LveTerrain::Vertex::getAttributeDescriptions() {
   std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
   return attributeDescriptions;
}

}  // namespace lve
