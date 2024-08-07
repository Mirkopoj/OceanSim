#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>

#include "../apps/second_app_frame_info.hpp"
#include "../lve/lve_device.hpp"
#include "../lve/lve_pipeline.hpp"

namespace lve {

class WaterRenderSystem {
  public:
   enum class PipeLineType {
      Normal,
      WireFrame,
   };

   WaterRenderSystem(LveDevice &device, VkRenderPass renderPass,
                       VkDescriptorSetLayout globalSetLayout,
                       const std::string &vertFilepath,
                       const std::string &fragFilepath,
                       const std::string &tesCFilepath,
                       const std::string &tesEFilepath,
                       VkDescriptorSet dispDesc,
                       VkDescriptorSetLayout dispLay);
   ~WaterRenderSystem();

   WaterRenderSystem(const WaterRenderSystem &) = delete;
   WaterRenderSystem &operator=(const WaterRenderSystem &) = delete;

   void renderTerrain(FrameInfo &frameInfo, PipeLineType pipeline);

  private:
   void createPipelineLayout(VkDescriptorSetLayout globalSetLayout,
                             VkDescriptorSetLayout dispSetLayout);
   void createPipeline(VkRenderPass renderPass,
                       const std::string &vertFilepath,
                       const std::string &fragFilepath,
                       const std::string &tesCFilepath,
                       const std::string &tesEFilepath,
                       PipeLineType pipeline);

   LveDevice &lveDevice;

   VkDescriptorSet displacementDesciptor;

   std::unique_ptr<LvePipeline> lvePipeline[2];
   VkPipelineLayout pipelineLayout;
};
}  // namespace lve
