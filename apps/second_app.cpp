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
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
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

float jonswap_alpha(float g, float fetch, float windSpeed) {
   return 0.076f * std::pow(g * fetch / windSpeed / windSpeed, -0.22f);
}

float jonswap_peak_features(float g, float fetch, float windSpeed) {
   return 22 * std::pow(windSpeed * fetch / g / g, -0.33f);
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

   MyTextureData buterfly(logN, N, 4, lveDevice,
                          VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData H0K(N, N, 2, lveDevice, VK_FORMAT_R16G16_SFLOAT);
   MyTextureData WavesData0(N, N, 4, lveDevice,
                            VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData H00(N, N, 4, lveDevice, VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData WavesData1(N, N, 4, lveDevice,
                            VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData H01(N, N, 4, lveDevice, VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData WavesData2(N, N, 4, lveDevice,
                            VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData H02(N, N, 4, lveDevice, VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData WavesData3(N, N, 4, lveDevice,
                            VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData H03(N, N, 4, lveDevice, VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData DxDzDyDxz(N, N, 4, lveDevice,
                           VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData DyxDyzDxxDzz(N, N, 4, lveDevice,
                              VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData ping_pong(N, N, 4, lveDevice,
                           VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData Displacement_Turbulence0(N, N, 4, lveDevice,
                                          VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData Derivatives0(N, N, 4, lveDevice,
                              VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData Displacement_Turbulence1(N, N, 4, lveDevice,
                                          VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData Derivatives1(N, N, 4, lveDevice,
                              VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData Displacement_Turbulence2(N, N, 4, lveDevice,
                                          VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData Derivatives2(N, N, 4, lveDevice,
                              VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData Displacement_Turbulence3(N, N, 4, lveDevice,
                                          VK_FORMAT_R16G16B16A16_SFLOAT);
   MyTextureData Derivatives3(N, N, 4, lveDevice,
                              VK_FORMAT_R16G16B16A16_SFLOAT);

   typedef struct {
      glm::float32 LengthScale;
      glm::float32 CutoffHigh;
      glm::float32 CutoffLow;
      glm::float32 GravityAcceleration;
      glm::float32 Depth;
      glm::uint Size;
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
       lveDevice, sizeof(comp_ubo), 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
   auto bufferInfo = compBuffer->descriptorInfo();

   LveDescriptorWriter(*gen_butterfly_desc_layout, *computePool)
       .writeImage(0, &butterflyImageInfo)
       .writeBuffer(1, &bufferInfo)
       .build(buterflyDescriptorSet);

   comp_ubo comp_buf[4];
   comp_buf[0].Size = N;
   comp_buf[0].LengthScale = 1279.0;
   comp_buf[0].GravityAcceleration = 9.81;
   comp_buf[0].Depth = 500.0;
   comp_buf[1].Size = N;
   comp_buf[1].LengthScale = 255.0;
   comp_buf[1].GravityAcceleration = 9.81;
   comp_buf[1].Depth = 500.0;
   comp_buf[2].Size = N;
   comp_buf[2].LengthScale = 17.0;
   comp_buf[2].GravityAcceleration = 9.81;
   comp_buf[2].Depth = 500.0;
   comp_buf[3].Size = N;
   comp_buf[3].LengthScale = 5.0;
   comp_buf[3].GravityAcceleration = 9.81;
   comp_buf[3].Depth = 500.0;

   float boundary1 = glm::pi<float>() / comp_buf[1].LengthScale * 6.f;
   float boundary2 = glm::pi<float>() / comp_buf[2].LengthScale * 6.f;
   float boundary3 = glm::pi<float>() / comp_buf[3].LengthScale * 6.f;

   comp_buf[0].CutoffLow = 0.0001f;
   comp_buf[0].CutoffHigh = boundary1;
   comp_buf[1].CutoffLow = boundary1;
   comp_buf[1].CutoffHigh = boundary2;
   comp_buf[2].CutoffLow = boundary2;
   comp_buf[2].CutoffHigh = boundary3;
   comp_buf[3].CutoffLow = boundary3;
   comp_buf[3].CutoffHigh = 9999.0;
   compBuffer->map();
   compBuffer->writeToBuffer(comp_buf);
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
           .addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .build();

   VkDescriptorImageInfo H0KImageInfo = {
       .imageView = H0K.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo WavesDataImageInfo0 = {
       .imageView = WavesData0.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo WavesDataImageInfo1 = {
       .imageView = WavesData1.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo WavesDataImageInfo2 = {
       .imageView = WavesData2.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo WavesDataImageInfo3 = {
       .imageView = WavesData3.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

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

   std::unique_ptr<LveBuffer> specBuf =
       std::make_unique<LveBuffer>(lveDevice, sizeof(SpectrumParameters),
                                   2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
   auto specBufferInfo = specBuf->descriptorInfo();

   auto bufferInfo0 =
       compBuffer->descriptorInfo(sizeof(comp_ubo), 0 * sizeof(comp_ubo));
   VkDescriptorSet init_spec_desc_set0 = {};
   LveDescriptorWriter(*init_spec_desc_lay, *computePool)
       .writeImage(0, &H0KImageInfo)
       .writeImage(1, &WavesDataImageInfo0)
       .writeBuffer(2, &specBufferInfo)
       .writeBuffer(3, &bufferInfo0)
       .build(init_spec_desc_set0);

   auto bufferInfo1 =
       compBuffer->descriptorInfo(sizeof(comp_ubo), 1 * sizeof(comp_ubo));
   VkDescriptorSet init_spec_desc_set1 = {};
   LveDescriptorWriter(*init_spec_desc_lay, *computePool)
       .writeImage(0, &H0KImageInfo)
       .writeImage(1, &WavesDataImageInfo1)
       .writeBuffer(2, &specBufferInfo)
       .writeBuffer(3, &bufferInfo1)
       .build(init_spec_desc_set1);

   auto bufferInfo2 =
       compBuffer->descriptorInfo(sizeof(comp_ubo), 2 * sizeof(comp_ubo));
   VkDescriptorSet init_spec_desc_set2 = {};
   LveDescriptorWriter(*init_spec_desc_lay, *computePool)
       .writeImage(0, &H0KImageInfo)
       .writeImage(1, &WavesDataImageInfo2)
       .writeBuffer(2, &specBufferInfo)
       .writeBuffer(3, &bufferInfo2)
       .build(init_spec_desc_set2);

   auto bufferInfo3 =
       compBuffer->descriptorInfo(sizeof(comp_ubo), 3 * sizeof(comp_ubo));
   VkDescriptorSet init_spec_desc_set3 = {};
   LveDescriptorWriter(*init_spec_desc_lay, *computePool)
       .writeImage(0, &H0KImageInfo)
       .writeImage(1, &WavesDataImageInfo3)
       .writeBuffer(2, &specBufferInfo)
       .writeBuffer(3, &bufferInfo3)
       .build(init_spec_desc_set3);

   SpectrumConfig spec_conf[2];
   spec_conf[0].scale = 1;
   spec_conf[0].windSpeed = 15;
   spec_conf[0].windDirection = -0.5;
   spec_conf[0].fetch = 100000;
   spec_conf[0].spreadBlend = 1;
   spec_conf[0].swell = 0.198;
   spec_conf[0].peakEnhancement = 3.3;
   spec_conf[0].shortWavesFade = 0.01;

   spec_conf[1].scale = 0;
   spec_conf[1].windSpeed = 1;
   spec_conf[1].windDirection = 0;
   spec_conf[1].fetch = 300000;
   spec_conf[1].spreadBlend = 1;
   spec_conf[1].swell = 1;
   spec_conf[1].peakEnhancement = 3.3;
   spec_conf[1].shortWavesFade = 0.01;
   SpectrumConfig new_conf[2];
   new_conf[0] = spec_conf[0];
   new_conf[1] = spec_conf[1];

   SpectrumParameters spec_params[2];
   spec_params[0].scale = spec_conf[0].scale;
   spec_params[0].angle = spec_conf[0].windDirection;
   spec_params[0].spreadBlend = spec_conf[0].spreadBlend;
   spec_params[0].swell = spec_conf[0].swell;
   spec_params[0].alpha =
       jonswap_alpha(comp_buf[0].GravityAcceleration, spec_conf[0].fetch,
                     spec_conf[0].windSpeed);
   spec_params[0].peakOmega =
       jonswap_peak_features(comp_buf[0].GravityAcceleration,
                             spec_conf[0].fetch, spec_conf[0].windSpeed);
   spec_params[0].gamma = spec_conf[0].peakEnhancement;
   spec_params[0].shortWavesFade = spec_conf[0].shortWavesFade;

   spec_params[1].scale = spec_conf[1].scale;
   spec_params[1].angle = spec_conf[1].windDirection;
   spec_params[1].spreadBlend = spec_conf[1].spreadBlend;
   spec_params[1].swell = spec_conf[1].swell;
   spec_params[1].alpha =
       jonswap_alpha(comp_buf[0].GravityAcceleration, spec_conf[1].fetch,
                     spec_conf[1].windSpeed);
   spec_params[1].peakOmega =
       jonswap_peak_features(comp_buf[0].GravityAcceleration,
                             spec_conf[1].fetch, spec_conf[1].windSpeed);
   spec_params[1].gamma = spec_conf[1].peakEnhancement;
   spec_params[1].shortWavesFade = spec_conf[1].shortWavesFade;
   specBuf->map();
   specBuf->writeToBuffer(spec_params);
   specBuf->flush();

   ComputeSystem init_spec{lveDevice,
                           {init_spec_desc_lay->getDescriptorSetLayout()},
                           "obj/shaders/init_spectrum.comp.spv"};

   std::unique_ptr<LveDescriptorSetLayout> conj_spec_desc_lay =
       LveDescriptorSetLayout::Builder(lveDevice)
           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_COMPUTE_BIT)
           .build();

   VkDescriptorImageInfo H0ImageInfo0 = {
       .imageView = H00.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo H0ImageInfo1 = {
       .imageView = H01.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo H0ImageInfo2 = {
       .imageView = H02.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo H0ImageInfo3 = {
       .imageView = H03.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorSet conj_spec_desc_set0 = {};
   LveDescriptorWriter(*conj_spec_desc_lay, *computePool)
       .writeImage(0, &H0KImageInfo)
       .writeImage(1, &H0ImageInfo0)
       .writeBuffer(2, &bufferInfo)
       .build(conj_spec_desc_set0);

   VkDescriptorSet conj_spec_desc_set1 = {};
   LveDescriptorWriter(*conj_spec_desc_lay, *computePool)
       .writeImage(0, &H0KImageInfo)
       .writeImage(1, &H0ImageInfo1)
       .writeBuffer(2, &bufferInfo)
       .build(conj_spec_desc_set1);

   VkDescriptorSet conj_spec_desc_set2 = {};
   LveDescriptorWriter(*conj_spec_desc_lay, *computePool)
       .writeImage(0, &H0KImageInfo)
       .writeImage(1, &H0ImageInfo2)
       .writeBuffer(2, &bufferInfo)
       .build(conj_spec_desc_set2);

   VkDescriptorSet conj_spec_desc_set3 = {};
   LveDescriptorWriter(*conj_spec_desc_lay, *computePool)
       .writeImage(0, &H0KImageInfo)
       .writeImage(1, &H0ImageInfo3)
       .writeBuffer(2, &bufferInfo)
       .build(conj_spec_desc_set3);

   ComputeSystem conj_spec{lveDevice,
                           {conj_spec_desc_lay->getDescriptorSetLayout()},
                           "obj/shaders/conj_spectrum.comp.spv"};

   init_spec.instant_dispatch(N, N, 1, init_spec_desc_set0);
   conj_spec.instant_dispatch(N, N, 1, conj_spec_desc_set0);
   init_spec.instant_dispatch(N, N, 1, init_spec_desc_set1);
   conj_spec.instant_dispatch(N, N, 1, conj_spec_desc_set1);
   init_spec.instant_dispatch(N, N, 1, init_spec_desc_set2);
   conj_spec.instant_dispatch(N, N, 1, conj_spec_desc_set2);
   init_spec.instant_dispatch(N, N, 1, init_spec_desc_set3);
   conj_spec.instant_dispatch(N, N, 1, conj_spec_desc_set3);

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
                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT |
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
           .build();

   VkDescriptorSet perm_inv_desc_set_1 = {};
   LveDescriptorWriter(*perm_inv_desc_lay, *computePool)
       .writeImage(0, &DxDzDyDxzImageInfo)
       .build(perm_inv_desc_set_1);

   VkDescriptorSet perm_inv_desc_set_2 = {};
   LveDescriptorWriter(*perm_inv_desc_lay, *computePool)
       .writeImage(0, &DyxDyzDxxDzzImageInfo)
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

   VkDescriptorSet timed_spec_desc_set0 = {};
   LveDescriptorWriter(*timed_spec_desc_lay, *computePool)
       .writeImage(0, &H0ImageInfo0)
       .writeImage(1, &WavesDataImageInfo0)
       .writeImage(2, &DxDzDyDxzImageInfo)
       .writeImage(3, &DyxDyzDxxDzzImageInfo)
       .writeBuffer(4, &lambdaBufferInfo)
       .build(timed_spec_desc_set0);

   VkDescriptorSet timed_spec_desc_set1 = {};
   LveDescriptorWriter(*timed_spec_desc_lay, *computePool)
       .writeImage(0, &H0ImageInfo1)
       .writeImage(1, &WavesDataImageInfo1)
       .writeImage(2, &DxDzDyDxzImageInfo)
       .writeImage(3, &DyxDyzDxxDzzImageInfo)
       .writeBuffer(4, &lambdaBufferInfo)
       .build(timed_spec_desc_set1);

   VkDescriptorSet timed_spec_desc_set2 = {};
   LveDescriptorWriter(*timed_spec_desc_lay, *computePool)
       .writeImage(0, &H0ImageInfo2)
       .writeImage(1, &WavesDataImageInfo2)
       .writeImage(2, &DxDzDyDxzImageInfo)
       .writeImage(3, &DyxDyzDxxDzzImageInfo)
       .writeBuffer(4, &lambdaBufferInfo)
       .build(timed_spec_desc_set2);

   VkDescriptorSet timed_spec_desc_set3 = {};
   LveDescriptorWriter(*timed_spec_desc_lay, *computePool)
       .writeImage(0, &H0ImageInfo3)
       .writeImage(1, &WavesDataImageInfo3)
       .writeImage(2, &DxDzDyDxzImageInfo)
       .writeImage(3, &DyxDyzDxxDzzImageInfo)
       .writeBuffer(4, &lambdaBufferInfo)
       .build(timed_spec_desc_set3);

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

   VkDescriptorImageInfo Displacement_TurbulenceImageInfo0 = {
       .sampler = Displacement_Turbulence0.Sampler,
       .imageView = Displacement_Turbulence0.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };
   VkDescriptorImageInfo DerivativesImageInfo0 = {
       .sampler = Derivatives0.Sampler,
       .imageView = Derivatives0.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo Displacement_TurbulenceImageInfo1 = {
       .sampler = Displacement_Turbulence1.Sampler,
       .imageView = Displacement_Turbulence1.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };
   VkDescriptorImageInfo DerivativesImageInfo1 = {
       .sampler = Derivatives1.Sampler,
       .imageView = Derivatives1.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo Displacement_TurbulenceImageInfo2 = {
       .sampler = Displacement_Turbulence2.Sampler,
       .imageView = Displacement_Turbulence2.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };
   VkDescriptorImageInfo DerivativesImageInfo2 = {
       .sampler = Derivatives2.Sampler,
       .imageView = Derivatives2.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorImageInfo Displacement_TurbulenceImageInfo3 = {
       .sampler = Displacement_Turbulence3.Sampler,
       .imageView = Displacement_Turbulence3.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };
   VkDescriptorImageInfo DerivativesImageInfo3 = {
       .sampler = Derivatives3.Sampler,
       .imageView = Derivatives3.ImageView,
       .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };

   VkDescriptorSet text_merg_desc_set0 = {};
   LveDescriptorWriter(*text_merg_desc_lay, *computePool)
       .writeImage(0, &DxDzDyDxzImageInfo)
       .writeImage(1, &DyxDyzDxxDzzImageInfo)
       .writeImage(2, &Displacement_TurbulenceImageInfo0)
       .writeImage(3, &DerivativesImageInfo0)
       .writeBuffer(4, &lambdaBufferInfo)
       .build(text_merg_desc_set0);

   VkDescriptorSet text_merg_desc_set1 = {};
   LveDescriptorWriter(*text_merg_desc_lay, *computePool)
       .writeImage(0, &DxDzDyDxzImageInfo)
       .writeImage(1, &DyxDyzDxxDzzImageInfo)
       .writeImage(2, &Displacement_TurbulenceImageInfo1)
       .writeImage(3, &DerivativesImageInfo1)
       .writeBuffer(4, &lambdaBufferInfo)
       .build(text_merg_desc_set1);

   VkDescriptorSet text_merg_desc_set2 = {};
   LveDescriptorWriter(*text_merg_desc_lay, *computePool)
       .writeImage(0, &DxDzDyDxzImageInfo)
       .writeImage(1, &DyxDyzDxxDzzImageInfo)
       .writeImage(2, &Displacement_TurbulenceImageInfo2)
       .writeImage(3, &DerivativesImageInfo2)
       .writeBuffer(4, &lambdaBufferInfo)
       .build(text_merg_desc_set2);

   VkDescriptorSet text_merg_desc_set3 = {};
   LveDescriptorWriter(*text_merg_desc_lay, *computePool)
       .writeImage(0, &DxDzDyDxzImageInfo)
       .writeImage(1, &DyxDyzDxxDzzImageInfo)
       .writeImage(2, &Displacement_TurbulenceImageInfo3)
       .writeImage(3, &DerivativesImageInfo3)
       .writeBuffer(4, &lambdaBufferInfo)
       .build(text_merg_desc_set3);

   ComputeSystem tex_merg{lveDevice,
                          {text_merg_desc_lay->getDescriptorSetLayout()},
                          "obj/shaders/texture_merger.comp.spv"};

   lambda_buff lamda_buf;
   lamda_buf.lambda = 1.0f;

   MyTextureData* imgs[18];
   imgs[0] = &buterfly;
   imgs[1] = &H0K;
   imgs[2] = &WavesData0;
   imgs[3] = &WavesData1;
   imgs[4] = &WavesData2;
   imgs[5] = &WavesData3;
   imgs[6] = &H00;
   imgs[7] = &H01;
   imgs[8] = &H02;
   imgs[9] = &H03;
   imgs[10] = &Displacement_Turbulence0;
   imgs[11] = &Displacement_Turbulence1;
   imgs[12] = &Displacement_Turbulence2;
   imgs[13] = &Displacement_Turbulence3;
   imgs[14] = &Derivatives0;
   imgs[15] = &Derivatives1;
   imgs[16] = &Derivatives2;
   imgs[17] = &Derivatives3;

   std::unique_ptr<LveDescriptorSetLayout> disp_desc_set_lay =
       LveDescriptorSetLayout::Builder(lveDevice)
           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                       VK_SHADER_STAGE_ALL_GRAPHICS)
           .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_ALL_GRAPHICS)
           .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_ALL_GRAPHICS)
           .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_ALL_GRAPHICS)
           .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_ALL_GRAPHICS)
           .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_ALL_GRAPHICS)
           .addBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_ALL_GRAPHICS)
           .addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_ALL_GRAPHICS)
           .addBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_ALL_GRAPHICS)
           .build();

   VkDescriptorSet disp_desc_set = {};
   LveDescriptorWriter(*disp_desc_set_lay, *computePool)
       .writeBuffer(0, &bufferInfo)
       .writeImage(1, &Displacement_TurbulenceImageInfo0)
       .writeImage(2, &DerivativesImageInfo0)
       .writeImage(3, &Displacement_TurbulenceImageInfo1)
       .writeImage(4, &DerivativesImageInfo1)
       .writeImage(5, &Displacement_TurbulenceImageInfo2)
       .writeImage(6, &DerivativesImageInfo2)
       .writeImage(7, &Displacement_TurbulenceImageInfo3)
       .writeImage(8, &DerivativesImageInfo3)
       .build(disp_desc_set);

   TerrainRenderSystem terrainRenderSystem{
       lveDevice,
       lveRenderer.getSwapChainRenderPass(),
       globalSetLayout->getDescriptorSetLayout(),
       "obj/shaders/water_shader.vert.spv",
       "obj/shaders/water_shader.frag.spv",
       "obj/shaders/water_shader.tesc.spv",
       "obj/shaders/water_shader.tese.spv",
       disp_desc_set,
       disp_desc_set_lay->getDescriptorSetLayout()};

   VkCommandBuffer computeCommandBuffer;
   VkCommandBufferAllocateInfo allocInfo = {};
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.commandPool = lveDevice.getCommandPool();
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandBufferCount = 1;

   vkAllocateCommandBuffers(lveDevice.device(), &allocInfo,
                            &computeCommandBuffer);

   VkCommandBufferBeginInfo beginInfo = {};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

   vkBeginCommandBuffer(computeCommandBuffer, &beginInfo);

   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     timed_spec.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           timed_spec.get_pipeline_layout(), 0, 1,
                           &timed_spec_desc_set0, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   VkMemoryBarrier memoryBarrier = {};
   memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
   memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
   memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     h_butterfly.get_pipeline());
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_1 : butterfly_desc_set_2_1;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          h_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_2 : butterfly_desc_set_2_2;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          h_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     v_butterfly.get_pipeline());
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_1 : butterfly_desc_set_2_1;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          v_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_2 : butterfly_desc_set_2_2;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          v_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     perm_inv.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           perm_inv.get_pipeline_layout(), 0, 1,
                           &perm_inv_desc_set_1, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           perm_inv.get_pipeline_layout(), 0, 1,
                           &perm_inv_desc_set_2, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     tex_merg.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           tex_merg.get_pipeline_layout(), 0, 1,
                           &text_merg_desc_set0, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     timed_spec.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           timed_spec.get_pipeline_layout(), 0, 1,
                           &timed_spec_desc_set1, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     h_butterfly.get_pipeline());
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_1 : butterfly_desc_set_2_1;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          h_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_2 : butterfly_desc_set_2_2;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          h_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     v_butterfly.get_pipeline());
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_1 : butterfly_desc_set_2_1;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          v_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_2 : butterfly_desc_set_2_2;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          v_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     perm_inv.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           perm_inv.get_pipeline_layout(), 0, 1,
                           &perm_inv_desc_set_1, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           perm_inv.get_pipeline_layout(), 0, 1,
                           &perm_inv_desc_set_2, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     tex_merg.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           tex_merg.get_pipeline_layout(), 0, 1,
                           &text_merg_desc_set1, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     timed_spec.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           timed_spec.get_pipeline_layout(), 0, 1,
                           &timed_spec_desc_set2, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     h_butterfly.get_pipeline());
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_1 : butterfly_desc_set_2_1;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          h_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_2 : butterfly_desc_set_2_2;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          h_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     v_butterfly.get_pipeline());
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_1 : butterfly_desc_set_2_1;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          v_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_2 : butterfly_desc_set_2_2;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          v_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     perm_inv.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           perm_inv.get_pipeline_layout(), 0, 1,
                           &perm_inv_desc_set_1, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           perm_inv.get_pipeline_layout(), 0, 1,
                           &perm_inv_desc_set_2, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     tex_merg.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           tex_merg.get_pipeline_layout(), 0, 1,
                           &text_merg_desc_set2, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     timed_spec.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           timed_spec.get_pipeline_layout(), 0, 1,
                           &timed_spec_desc_set3, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     h_butterfly.get_pipeline());
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_1 : butterfly_desc_set_2_1;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          h_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_2 : butterfly_desc_set_2_2;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          h_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     v_butterfly.get_pipeline());
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_1 : butterfly_desc_set_2_1;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          v_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   for (size_t i = 0; i < logN; ++i) {
      vkCmdUpdateBuffer(computeCommandBuffer, stageBuffer->getBuffer(), 0,
                        sizeof(uint32_t), &i);
      VkBufferMemoryBarrier bufferBarrier = {};
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.buffer = stageBuffer->getBuffer();
      bufferBarrier.offset = 0;
      bufferBarrier.size = VK_WHOLE_SIZE;
      VkDescriptorSet desc =
          i % 2 == 0 ? butterfly_desc_set_1_2 : butterfly_desc_set_2_2;
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
      vkCmdBindDescriptorSets(
          computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          v_butterfly.get_pipeline_layout(), 0, 1, &desc, 0, nullptr);
      vkCmdDispatch(computeCommandBuffer, N, N, 1);
      vkCmdPipelineBarrier(
          computeCommandBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
          0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   }
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     perm_inv.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           perm_inv.get_pipeline_layout(), 0, 1,
                           &perm_inv_desc_set_1, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           perm_inv.get_pipeline_layout(), 0, 1,
                           &perm_inv_desc_set_2, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
   vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                     tex_merg.get_pipeline());
   vkCmdBindDescriptorSets(computeCommandBuffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           tex_merg.get_pipeline_layout(), 0, 1,
                           &text_merg_desc_set3, 0, nullptr);
   vkCmdDispatch(computeCommandBuffer, N, N, 1);
   vkCmdPipelineBarrier(
       computeCommandBuffer,
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

   vkEndCommandBuffer(computeCommandBuffer);

   float time = 0;
   float angle = 3.15;
   float colors[3][4] = {
       {0.98823529412f, 0.97637058824f, 0.72941176471f, 0.5f},
       {0.0f, 0.11764705882f, 1.0f, 1.0f},
       {0.0f, 0.0f, 1.0f, 1.0f}};
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
         myimgui.update(cameraController, caminata, pipeline,
                        viewerObject.transform.translation, frameTime,
                        imgs, new_conf, angle, colors);

         time += frameTime;
         GlobalUbo ubo{};
         ubo.projection = camera.getProjection();
         ubo.view = camera.getView();
         ubo.inverseView = camera.getInverseView();
         ubo.cols = xn;
         ubo.time = time;
         ubo.lightPosition =
             glm::vec3{1.0, glm::sin(angle), glm::cos(angle)};
         ubo.sunColor.r = colors[0][0];
         ubo.sunColor.g = colors[0][1];
         ubo.sunColor.b = colors[0][2];
         ubo.sunColor.a = colors[0][3];
         ubo.scatterColor.r = colors[1][0];
         ubo.scatterColor.g = colors[1][1];
         ubo.scatterColor.b = colors[1][2];
         ubo.bubbleColor.r = colors[2][0];
         ubo.bubbleColor.g = colors[2][1];
         ubo.bubbleColor.b = colors[2][2];

         uboBuffers[frameIndex]->writeToBuffer(&ubo);
         uboBuffers[frameIndex]->flush();

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

         VkSubmitInfo submitInfo = {};
         submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
         submitInfo.commandBufferCount = 1;
         submitInfo.pCommandBuffers = &computeCommandBuffer;
         vkQueueSubmit(lveDevice.computeQueue(), 1, &submitInfo,
                       VK_NULL_HANDLE);
         vkQueueWaitIdle(lveDevice.computeQueue());

         if (new_conf[0].scale != spec_conf[0].scale ||
             new_conf[0].windSpeed != spec_conf[0].windSpeed ||
             new_conf[0].windDirection != spec_conf[0].windDirection ||
             new_conf[0].fetch != spec_conf[0].fetch ||
             new_conf[0].spreadBlend != spec_conf[0].spreadBlend ||
             new_conf[0].swell != spec_conf[0].swell ||
             new_conf[0].peakEnhancement != spec_conf[0].peakEnhancement ||
             new_conf[0].shortWavesFade != spec_conf[0].shortWavesFade ||
             new_conf[1].scale != spec_conf[1].scale ||
             new_conf[1].windSpeed != spec_conf[1].windSpeed ||
             new_conf[1].windDirection != spec_conf[1].windDirection ||
             new_conf[1].fetch != spec_conf[1].fetch ||
             new_conf[1].spreadBlend != spec_conf[1].spreadBlend ||
             new_conf[1].swell != spec_conf[1].swell ||
             new_conf[1].peakEnhancement != spec_conf[1].peakEnhancement ||
             new_conf[1].shortWavesFade != spec_conf[1].shortWavesFade) {
            spec_conf[0] = new_conf[0];
            spec_conf[1] = new_conf[1];
            spec_params[0].scale = spec_conf[0].scale;
            spec_params[0].angle = spec_conf[0].windDirection;
            spec_params[0].spreadBlend = spec_conf[0].spreadBlend;
            spec_params[0].swell = spec_conf[0].swell;
            spec_params[0].alpha =
                jonswap_alpha(comp_buf[0].GravityAcceleration,
                              spec_conf[0].fetch, spec_conf[0].windSpeed);
            spec_params[0].peakOmega = jonswap_peak_features(
                comp_buf[0].GravityAcceleration, spec_conf[0].fetch,
                spec_conf[0].windSpeed);
            spec_params[0].gamma = spec_conf[0].peakEnhancement;
            spec_params[0].shortWavesFade = spec_conf[0].shortWavesFade;

            spec_params[1].scale = spec_conf[1].scale;
            spec_params[1].angle = spec_conf[1].windDirection;
            spec_params[1].spreadBlend = spec_conf[1].spreadBlend;
            spec_params[1].swell = spec_conf[1].swell;
            spec_params[1].alpha =
                jonswap_alpha(comp_buf[0].GravityAcceleration,
                              spec_conf[1].fetch, spec_conf[1].windSpeed);
            spec_params[1].peakOmega = jonswap_peak_features(
                comp_buf[0].GravityAcceleration, spec_conf[1].fetch,
                spec_conf[1].windSpeed);
            spec_params[1].gamma = spec_conf[1].peakEnhancement;
            spec_params[1].shortWavesFade = spec_conf[1].shortWavesFade;
            specBuf->writeToBuffer(spec_params);
            specBuf->flush();
            init_spec.instant_dispatch(N, N, 1, init_spec_desc_set0);
            conj_spec.instant_dispatch(N, N, 1, conj_spec_desc_set0);
            init_spec.instant_dispatch(N, N, 1, init_spec_desc_set1);
            conj_spec.instant_dispatch(N, N, 1, conj_spec_desc_set1);
            init_spec.instant_dispatch(N, N, 1, init_spec_desc_set2);
            conj_spec.instant_dispatch(N, N, 1, conj_spec_desc_set2);
            init_spec.instant_dispatch(N, N, 1, init_spec_desc_set3);
            conj_spec.instant_dispatch(N, N, 1, conj_spec_desc_set3);
         }
      }
   }

   vkDeviceWaitIdle(lveDevice.device());
}

void SecondApp::loadGameObjects() {
   yn = N;
   xn = N;
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
   viewerObject.transform.translation.x = 5 * static_cast<float>(xn - 1) / 2.f;
   viewerObject.transform.translation.z = 5 * static_cast<float>(yn - 1) / 2.f;
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
