#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <string>
#include <vector>

#include "lve_device.hpp"

namespace lve {

struct PipelineConfigInfo {
   PipelineConfigInfo() = default;
   PipelineConfigInfo(const PipelineConfigInfo &) = delete;
   PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

   std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
   std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
   VkPipelineViewportStateCreateInfo viewportInfo;
   VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
   VkPipelineRasterizationStateCreateInfo rasterizationInfo;
   VkPipelineMultisampleStateCreateInfo multisampleInfo;
   VkPipelineColorBlendAttachmentState colorBlendAttachment;
   VkPipelineColorBlendStateCreateInfo colorBlendInfo;
   VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
   std::vector<VkDynamicState> dynamicStateEnables;
   VkPipelineDynamicStateCreateInfo dynamicStateInfo;
   VkPipelineTessellationStateCreateInfo tessellationStateInfo;
   VkPipelineLayout pipelineLayout = nullptr;
   VkRenderPass renderPass = nullptr;
   uint32_t subpass = 0;
};

class LvePipeline {
  public:
   LvePipeline(LveDevice &device, const std::string &vertFilepath,
               const std::string &fragFilepath,
               const std::string &tesCFilepath,
               const std::string &tesEFilepath,
               const PipelineConfigInfo &configInfo);

   ~LvePipeline();

   LvePipeline(const LvePipeline &) = delete;
   LvePipeline &operator=(const LvePipeline &) = delete;

   void bind(VkCommandBuffer commandBuffer);
   static void defaultPipelineConfigInfo(PipelineConfigInfo &configInfo);
   static void enableAlphaBlending(PipelineConfigInfo &configInfo);

   static void barrier(VkCommandBuffer &commandBuffer,
                VkPipelineStageFlags srcStageMask,
                VkPipelineStageFlags dstStageMask,
                VkAccessFlags srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                VkAccessFlags dstAccessMask = VK_ACCESS_SHADER_READ_BIT);

  private:
   static std::vector<char> readFile(const std::string &filepath);

   void createGraphicsPipeline(const std::string &vertFilepath,
                               const std::string &fragFilepath,
                               const std::string &tesCFilepath,
                               const std::string &tesEFilepath,
                               const PipelineConfigInfo &configInfo);

   void createShaderModule(const std::vector<char> &code,
                           VkShaderModule *shaderModule);

   LveDevice &lveDevice;
   VkPipeline graphicsPipeline;
   VkShaderModule vertShaderModule;
   VkShaderModule fragShaderModule;
   VkShaderModule tesCShaderModule;
   VkShaderModule tesEShaderModule;
};
}  // namespace lve
