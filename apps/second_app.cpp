#include "second_app.hpp"

#include <pthread.h>
#include <vulkan/vulkan_core.h>

// std
#include <imgui.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <glm/common.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <iostream>
#include <memory>
#include <vector>

#include "../lve/lve_buffer.hpp"
#include "../lve/lve_camera.hpp"
#include "../lve/lve_descriptors.hpp"
#include "../lve/lve_swap_chain.hpp"
#include "../lve/lve_water.hpp"
#include "../movement_controllers/terrain_movement_controller.hpp"
#include "../systems/gui_system.hpp"
#include "../systems/water_render_system.hpp"
#include "second_app_frame_info.hpp"
#include "systems/compute_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace lve {

SecondApp::SecondApp(size_t n) : N(n) {
   loadGameObjects();
}

SecondApp::~SecondApp() {
}

void SecondApp::run() {
   std::vector<std::unique_ptr<LveBuffer>> uboBuffers(
       LveSwapChain::MAX_FRAMES_IN_FLIGHT);
   for (int i = 0; i < uboBuffers.size(); i++) {
      uboBuffers[i] =
          std::make_unique<LveBuffer>(lveDevice, sizeof(GlobalUbo), 1,
                                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      uboBuffers[i]->map();
   }

   std::unique_ptr<LveDescriptorSetLayout> globalSetLayout =
       LveDescriptorSetLayout::Builder(lveDevice)
           .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                       VK_SHADER_STAGE_ALL_GRAPHICS)
           .build();

   std::vector<VkDescriptorSet> globalDescriptorSets(
       LveSwapChain::MAX_FRAMES_IN_FLIGHT);
   for (int i = 0; i < globalDescriptorSets.size(); i++) {
      auto bufferInfo = uboBuffers[i]->descriptorInfo();
      LveDescriptorWriter(*globalSetLayout, *globalPool)
          .writeBuffer(0, &bufferInfo)
          .build(globalDescriptorSets[i]);
   }

   TerrainRenderSystem terrainRenderSystem{
       lveDevice, lveRenderer.getSwapChainRenderPass(),
       globalSetLayout->getDescriptorSetLayout(),
       "obj/shaders/water_shader.vert.spv",
       "obj/shaders/water_shader.frag.spv"};

   LveCamera camera{};

   float cameraHeight = 2.f;
   LveGameObject viewerObject = LveGameObject::createGameObject();
   fixViewer(viewerObject, cameraHeight);

   TerrainMovementController cameraController{};

   ImGuiGui myimgui(lveWindow.getGLFWwindow(), lveDevice, lveRenderer,
                    imguiPool->descriptor_pool());

   auto currentTime = std::chrono::high_resolution_clock::now();
   bool caminata = false;
   bool viento = false;

   size_t pipeline = 0;

   size_t logN = std::log2(N);
   std::cout << "log2(N) = " << logN << "\n";

   MyTextureData buterfly(logN, N, 4, lveDevice,
                          VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData H0K(N, N, 2, lveDevice, VK_FORMAT_R16G16_SFLOAT);
   MyTextureData WavesData(N, N, 4, lveDevice,
                           VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData H0(N, N, 4, lveDevice, VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData DxDzDyDxz(N, N, 4, lveDevice,
                           VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData DyxDyzDxxDzz(N, N, 4, lveDevice,
                              VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData ping_pong(N, N, 4, lveDevice,
                           VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData Displacement_Turbulence(N, N, 4, lveDevice,
                                         VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData Derivatives(N, N, 4, lveDevice,
                           VK_FORMAT_R16G16B16A16_SFLOAT);

   typedef struct {
      glm::uint Size;
      glm::float32 LengthScale;
      glm::float32 CutoffHigh;
      glm::float32 CutoffLow;
      glm::float32 GravityAcceleration;
      glm::float32 Depth;
   } comp_ubo;

   std::unique_ptr<LveDescriptorSetLayout> gen_butterfly_desc_layout =
       LveDescriptorSetLayout::Builder(lveDevice)
           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .build();

   ComputeSystem gen_butterfly{
       lveDevice,
       {gen_butterfly_desc_layout->getDescriptorSetLayout()},
       "obj/shaders/gen_buterfly.comp.spv"};

   VkDescriptorImageInfo butterflyImageInfo = {
       .imageView = buterfly.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorSet buterflyDescriptorSet = {};
   std::unique_ptr<LveBuffer> compBuffer = std::make_unique<LveBuffer>(
       lveDevice, sizeof(comp_ubo), 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
   auto bufferInfo = compBuffer->descriptorInfo();

   LveDescriptorWriter(*gen_butterfly_desc_layout, *computePool)
       .writeImage(0, &butterflyImageInfo)
       .writeBuffer(1, &bufferInfo)
       .build(buterflyDescriptorSet);

   comp_ubo comp_buf;
   comp_buf.Size = N;
   comp_buf.LengthScale = 100.0;
   comp_buf.CutoffHigh = 100.0;
   comp_buf.CutoffLow = 0.1;
   comp_buf.GravityAcceleration = 9.8;
   comp_buf.Depth = 9.8;
   compBuffer->map();
   compBuffer->writeToBuffer(&comp_buf);
   compBuffer->unmap();

   gen_butterfly.instant_dispatch(std::log2(N), N / 2, 1,
                                  buterflyDescriptorSet);

   std::unique_ptr<LveDescriptorSetLayout> init_spec_desc_lay =
       LveDescriptorSetLayout::Builder(lveDevice)
           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .build();

   VkDescriptorImageInfo H0KImageInfo = {
       .imageView = H0K.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo WavesDataImageInfo = {
       .imageView = WavesData.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorSet init_spec_desc_set = {};
   std::unique_ptr<LveBuffer> specBuf =
       std::make_unique<LveBuffer>(lveDevice, sizeof(SpectrumParameters),
                                   2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
   auto specBufferInfo = specBuf->descriptorInfo();

   LveDescriptorWriter(*init_spec_desc_lay, *computePool)
       .writeImage(0, &H0KImageInfo)
       .writeImage(1, &WavesDataImageInfo)
       .writeBuffer(2, &specBufferInfo)
       .writeBuffer(3, &bufferInfo)
       .build(init_spec_desc_set);

   SpectrumParameters spec_params[2];
   spec_params[0].scale = 500.0;
   spec_params[0].angle = 0.0;
   spec_params[0].spreadBlend = 0.0;
   spec_params[0].swell = 150.0;
   spec_params[0].alpha = 170.0;
   spec_params[0].peakOmega = 0.0;
   spec_params[0].gamma = 1.0;
   spec_params[0].shortWavesFade = 0.0;
   spec_params[1] = spec_params[0];
   specBuf->map();
   specBuf->writeToBuffer(spec_params);
   specBuf->unmap();

   ComputeSystem init_spec{lveDevice,
                           {init_spec_desc_lay->getDescriptorSetLayout()},
                           "obj/shaders/init_spectrum.comp.spv"};

   init_spec.instant_dispatch(N, N, 1, init_spec_desc_set);

   std::unique_ptr<LveDescriptorSetLayout> conj_spec_desc_lay =
       LveDescriptorSetLayout::Builder(lveDevice)
           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .build();

   VkDescriptorImageInfo H0ImageInfo = {
       .imageView = H0.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorSet conj_spec_desc_set = {};

   LveDescriptorWriter(*conj_spec_desc_lay, *computePool)
       .writeImage(0, &H0KImageInfo)
       .writeImage(1, &H0ImageInfo)
       .writeBuffer(2, &bufferInfo)
       .build(conj_spec_desc_set);

   ComputeSystem conj_spec{lveDevice,
                           {conj_spec_desc_lay->getDescriptorSetLayout()},
                           "obj/shaders/conj_spectrum.comp.spv"};

   conj_spec.instant_dispatch(N, N, 1, conj_spec_desc_set);

   VkDescriptorImageInfo DxDzDyDxzImageInfo = {
       .imageView = DxDzDyDxz.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo DyxDyzDxxDzzImageInfo = {
       .imageView = DyxDyzDxxDzz.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   typedef struct {
      glm::float32 time;
      glm::float32 delta_time;
      glm::float32 lambda;
   } lambda_buff;

   std::unique_ptr<LveBuffer> lambdaBuffer =
       std::make_unique<LveBuffer>(lveDevice, sizeof(lambda_buff), 1,
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
   auto lambdaBufferInfo = lambdaBuffer->descriptorInfo();

   std::unique_ptr<LveDescriptorSetLayout> butterfly_desc_lay =
       LveDescriptorSetLayout::Builder(lveDevice)
           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .build();

   VkDescriptorImageInfo ping_pong_ImageInfo = {
       .imageView = ping_pong.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   std::unique_ptr<LveBuffer> stageBuffer =
       std::make_unique<LveBuffer>(lveDevice, sizeof(glm::int32), 1,
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
   auto stageBufferInfo = stageBuffer->descriptorInfo();

   VkDescriptorSet butterfly_desc_set_1_1 = {};
   LveDescriptorWriter(*butterfly_desc_lay, *computePool)
       .writeImage(0, &DxDzDyDxzImageInfo)
       .writeImage(1, &ping_pong_ImageInfo)
       .writeImage(2, &butterflyImageInfo)
       .writeBuffer(3, &stageBufferInfo)
       .build(butterfly_desc_set_1_1);

   VkDescriptorSet butterfly_desc_set_2_1 = {};
   LveDescriptorWriter(*butterfly_desc_lay, *computePool)
       .writeImage(0, &ping_pong_ImageInfo)
       .writeImage(1, &DxDzDyDxzImageInfo)
       .writeImage(2, &butterflyImageInfo)
       .writeBuffer(3, &stageBufferInfo)
       .build(butterfly_desc_set_2_1);

   VkDescriptorSet butterfly_desc_set_1_2 = {};
   LveDescriptorWriter(*butterfly_desc_lay, *computePool)
       .writeImage(0, &DyxDyzDxxDzzImageInfo)
       .writeImage(1, &ping_pong_ImageInfo)
       .writeImage(2, &butterflyImageInfo)
       .writeBuffer(3, &stageBufferInfo)
       .build(butterfly_desc_set_1_2);

   VkDescriptorSet butterfly_desc_set_2_2 = {};
   LveDescriptorWriter(*butterfly_desc_lay, *computePool)
       .writeImage(0, &ping_pong_ImageInfo)
       .writeImage(1, &DyxDyzDxxDzzImageInfo)
       .writeImage(2, &butterflyImageInfo)
       .writeBuffer(3, &stageBufferInfo)
       .build(butterfly_desc_set_2_2);

   ComputeSystem h_butterfly{
       lveDevice,
       {butterfly_desc_lay->getDescriptorSetLayout()},
       "obj/shaders/h_butterfly.comp.spv"};

   ComputeSystem v_butterfly{
       lveDevice,
       {butterfly_desc_lay->getDescriptorSetLayout()},
       "obj/shaders/v_butterfly.comp.spv"};

   std::unique_ptr<LveDescriptorSetLayout> perm_inv_desc_lay =
       LveDescriptorSetLayout::Builder(lveDevice)
           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .build();

   VkDescriptorSet perm_inv_desc_set_1 = {};
   LveDescriptorWriter(*perm_inv_desc_lay, *computePool)
       .writeImage(0, &DxDzDyDxzImageInfo)
       .writeBuffer(1, &bufferInfo)
       .build(perm_inv_desc_set_1);

   VkDescriptorSet perm_inv_desc_set_2 = {};
   LveDescriptorWriter(*perm_inv_desc_lay, *computePool)
       .writeImage(0, &DyxDyzDxxDzzImageInfo)
       .writeBuffer(1, &bufferInfo)
       .build(perm_inv_desc_set_2);

   ComputeSystem perm_inv{lveDevice,
                          {perm_inv_desc_lay->getDescriptorSetLayout()},
                          "obj/shaders/inv_perm.comp.spv"};

   std::unique_ptr<LveDescriptorSetLayout> timed_spec_desc_lay =
       LveDescriptorSetLayout::Builder(lveDevice)
           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .build();

   VkDescriptorSet timed_spec_desc_set = {};

   LveDescriptorWriter(*timed_spec_desc_lay, *computePool)
       .writeImage(0, &H0ImageInfo)
       .writeImage(1, &WavesDataImageInfo)
       .writeImage(2, &DxDzDyDxzImageInfo)
       .writeImage(3, &DyxDyzDxxDzzImageInfo)
       .writeBuffer(4, &lambdaBufferInfo)
       .build(timed_spec_desc_set);

   ComputeSystem timed_spec{
       lveDevice,
       {timed_spec_desc_lay->getDescriptorSetLayout()},
       "obj/shaders/timed_spectrum.comp.spv"};

   std::unique_ptr<LveDescriptorSetLayout> text_merg_desc_lay =
       LveDescriptorSetLayout::Builder(lveDevice)
           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .build();

   VkDescriptorImageInfo Displacement_TurbulenceImageInfo = {
       .imageView = Displacement_Turbulence.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo DerivativesImageInfo = {
       .imageView = Derivatives.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorSet text_merg_desc_set = {};

   LveDescriptorWriter(*text_merg_desc_lay, *computePool)
       .writeImage(0, &DxDzDyDxzImageInfo)
       .writeImage(1, &DyxDyzDxxDzzImageInfo)
       .writeImage(2, &Displacement_TurbulenceImageInfo)
       .writeImage(3, &DerivativesImageInfo)
       .writeBuffer(4, &lambdaBufferInfo)
       .build(text_merg_desc_set);

   ComputeSystem tex_merg{
       lveDevice,
       {text_merg_desc_lay->getDescriptorSetLayout()},
       "obj/shaders/texture_merger.comp.spv"};

   lambda_buff lamda_buf;
   lamda_buf.lambda = 1.0f;

   MyTextureData* imgs[6];
   imgs[0] = &buterfly;
   imgs[1] = &H0K;
   imgs[2] = &WavesData;
   imgs[3] = &H0;
   imgs[4] = &Displacement_Turbulence;
   imgs[5] = &Derivatives;

   float time = 0;
   while (!lveWindow.shouldClose()) {
      glfwPollEvents();

      auto newTime = std::chrono::high_resolution_clock::now();
      float frameTime =
          std::chrono::duration<float, std::chrono::seconds::period>(
              newTime - currentTime)
              .count();
      currentTime = newTime;

      cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime,
                                     viewerObject, altittudeMap,
                                     cameraHeight, caminata);

      camera.setViewYXZ(viewerObject.transform.translation,
                        viewerObject.transform.rotation);

      float aspect = lveRenderer.getAspectRatio();
      camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f,
                                      fmax(xn, yn) * 1.8);

      if (auto commandBuffer = lveRenderer.beginFrame()) {
         int frameIndex = lveRenderer.getFrameIndex();
         FrameInfo frameInfo{frameIndex,
                             frameTime,
                             commandBuffer,
                             camera,
                             globalDescriptorSets[frameIndex],
                             terrain};
         myimgui.new_frame();

         // update
         time += frameTime;
         GlobalUbo ubo{};
         ubo.projection = camera.getProjection();
         ubo.view = camera.getView();
         ubo.cols = xn;
         ubo.time = time;
         uboBuffers[frameIndex]->writeToBuffer(&ubo);
         uboBuffers[frameIndex]->flush();
         myimgui.update(cameraController, caminata, pipeline,
                        viewerObject.transform.translation, frameTime,
                        imgs, spec_params[0]);

         // render system
         lveRenderer.beginSwapChainRenderPass(commandBuffer);

         if (terrain) {
            terrainRenderSystem.renderTerrain(
                frameInfo,
                static_cast<TerrainRenderSystem::PipeLineType>(pipeline));
         }
         myimgui.render(commandBuffer);

         lveRenderer.endSwapChainRenderPass(commandBuffer);
         lveRenderer.endFrame();

         lamda_buf.time = ubo.time;
         lamda_buf.delta_time = frameTime;
         lambdaBuffer->map();
         lambdaBuffer->writeToBuffer(&lamda_buf);
         lambdaBuffer->unmap();
         timed_spec.instant_dispatch(N, N, 1, timed_spec_desc_set);

         for (int i = 0; i < logN; ++i) {
            VkDescriptorSet desc = i % 2 == 0 ? butterfly_desc_set_1_1
                                                : butterfly_desc_set_2_1;
            stageBuffer->map();
            stageBuffer->writeToBuffer(&i);
            stageBuffer->unmap();
            h_butterfly.instant_dispatch(N, N, 1, desc);
         }
         for (int i = 0; i < logN; ++i) {
            VkDescriptorSet desc = i % 2 == 0 ? butterfly_desc_set_1_2
                                                : butterfly_desc_set_2_2;
            stageBuffer->map();
            stageBuffer->writeToBuffer(&i);
            stageBuffer->unmap();
            h_butterfly.instant_dispatch(N, N, 1, desc);
         }
         for (int i = 0; i < logN; ++i) {
            VkDescriptorSet desc = i % 2 == 0 ? butterfly_desc_set_1_1
                                                : butterfly_desc_set_2_1;
            stageBuffer->map();
            stageBuffer->writeToBuffer(&i);
            stageBuffer->unmap();
            v_butterfly.instant_dispatch(N, N, 1, desc);
         }
         for (int i = 0; i < logN; ++i) {
            VkDescriptorSet desc = i % 2 == 0 ? butterfly_desc_set_1_2
                                                : butterfly_desc_set_2_2;
            stageBuffer->map();
            stageBuffer->writeToBuffer(&i);
            stageBuffer->unmap();
            v_butterfly.instant_dispatch(N, N, 1, desc);
         }
         perm_inv.instant_dispatch(N, N, 1, perm_inv_desc_set_1);
         perm_inv.instant_dispatch(N, N, 1, perm_inv_desc_set_2);
			tex_merg.instant_dispatch(N, N, 1, text_merg_desc_set);

         if (spec_params[0].scale != spec_params[1].scale ||
             spec_params[0].angle != spec_params[1].angle ||
             spec_params[0].spreadBlend != spec_params[1].spreadBlend ||
             spec_params[0].swell != spec_params[1].swell ||
             spec_params[0].alpha != spec_params[1].alpha ||
             spec_params[0].peakOmega != spec_params[1].peakOmega ||
             spec_params[0].gamma != spec_params[1].gamma ||
             spec_params[0].shortWavesFade !=
                 spec_params[1].shortWavesFade) {
            spec_params[1] = spec_params[0];
            specBuf->map();
            specBuf->writeToBuffer(spec_params);
            specBuf->unmap();
            init_spec.instant_dispatch(N, N, 1, init_spec_desc_set);
            conj_spec.instant_dispatch(N, N, 1, conj_spec_desc_set);
         }
      }
   }

   vkDeviceWaitIdle(lveDevice.device());
}

void SecondApp::loadGameObjects() {
   yn = 100;
   xn = 100;
   for (size_t x = 0; x < xn; ++x) {
      std::vector<glm::float32> row;
      for (size_t y = 0; y < yn; ++y) {
         row.push_back(0);
      }
      altittudeMap.push_back(row);
   };
   terrain = LveTerrain::createModel(lveDevice, xn, yn);
}

void SecondApp::fixViewer(LveGameObject& viewerObject,
                          float cameraHeight) {
   viewerObject.transform.translation.x = static_cast<float>(xn - 1) / 2.f;
   viewerObject.transform.translation.z = static_cast<float>(yn - 1) / 2.f;
   uint32_t x = glm::clamp(
       xn - (uint32_t)roundf(viewerObject.transform.translation.x),
       (uint32_t)0, xn - 1);
   uint32_t y =
       glm::clamp((uint32_t)roundf(viewerObject.transform.translation.z),
                  (uint32_t)0, yn - 1);
   if (xn && yn) {
      viewerObject.transform.translation.y =
          -cameraHeight - altittudeMap[y][x];
   }
}

}  // namespace lve
