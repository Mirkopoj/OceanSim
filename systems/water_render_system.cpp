#include "water_render_system.hpp"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <glm/fwd.hpp>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>

namespace lve {

WaterRenderSystem::WaterRenderSystem(
    LveDevice &device, VkRenderPass renderPass,
    VkDescriptorSetLayout globalSetLayout, const std::string &vertFilepath,
    const std::string &fragFilepath, const std::string &tesCFilepath,
    const std::string &tesEFilepath, VkDescriptorSet dispDesc,
    VkDescriptorSetLayout dispLay)
    : lveDevice{device}, displacementDesciptor{dispDesc} {
   createPipelineLayout(globalSetLayout, dispLay);
   createPipeline(renderPass, vertFilepath, fragFilepath, tesCFilepath,
                  tesEFilepath, PipeLineType::Normal);
   createPipeline(renderPass, vertFilepath, fragFilepath, tesCFilepath,
                  tesEFilepath, PipeLineType::WireFrame);
}

WaterRenderSystem::~WaterRenderSystem() {
   vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
}

void WaterRenderSystem::createPipelineLayout(
    VkDescriptorSetLayout globalSetLayout,
    VkDescriptorSetLayout dispSetLayout) {
   std::vector<VkDescriptorSetLayout> descriptoSetLayouts{globalSetLayout,
                                                          dispSetLayout};

   VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
   pipelineLayoutInfo.sType =
       VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount =
       static_cast<uint32_t>(descriptoSetLayouts.size());
   pipelineLayoutInfo.pSetLayouts = descriptoSetLayouts.data();

   if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo,
                              nullptr, &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
   }
}

void WaterRenderSystem::createPipeline(VkRenderPass renderPass,
                                         const std::string &vertFilepath,
                                         const std::string &fragFilepath,
                                         const std::string &tesCFilepath,
                                         const std::string &tesEFilepath,
                                         PipeLineType pipeline) {
   assert(pipelineLayout != nullptr &&
          "Cannot create pipeline before pipeline layout");

   PipelineConfigInfo pipelineConfig{};
   LvePipeline::defaultPipelineConfigInfo(pipelineConfig);
   pipelineConfig.inputAssemblyInfo.topology =
       VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

   if (pipeline == PipeLineType::WireFrame) {
      pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
      pipelineConfig.rasterizationInfo.lineWidth = 1.0f;
      pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
   }

   pipelineConfig.bindingDescriptions =
       LveWater::Vertex::getBindingDescriptions();
   pipelineConfig.attributeDescriptions =
       LveWater::Vertex::getAttributeDescriptions();
   pipelineConfig.renderPass = renderPass;
   pipelineConfig.pipelineLayout = pipelineLayout;
   lvePipeline[static_cast<size_t>(pipeline)] =
       std::make_unique<LvePipeline>(lveDevice, vertFilepath, fragFilepath,
                                     tesCFilepath, tesEFilepath,
                                     pipelineConfig);
}

void WaterRenderSystem::renderTerrain(FrameInfo &frameInfo,
                                        PipeLineType pipeline) {
   lvePipeline[static_cast<size_t>(pipeline)]->bind(
       frameInfo.commandBuffer);

   VkDescriptorSet descs[2] = {frameInfo.globalDescriptorSet,
                               displacementDesciptor};

   vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                           VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                           0, 2, descs, 0, nullptr);

   frameInfo.terrain->draw(frameInfo.commandBuffer);
}

}  // namespace lve
