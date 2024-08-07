#include "gui_system.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <vulkan/vulkan_core.h>

#include <cstdio>

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "lve/lve_renderer.hpp"

const char *vk_result_to_c_string(VkResult result);

static void check_vk_result(VkResult err);

ImGuiGui::ImGuiGui(GLFWwindow *window, lve::LveDevice &lveDevice,
                   lve::LveRenderer &lveRenderer,
                   VkDescriptorPool imguiPool) {
   ImGui_ImplVulkan_InitInfo info = {};
   info.Instance = lveDevice.get_instance();
   info.PhysicalDevice = lveDevice.physical_device();
   info.Device = lveDevice.device();
   info.QueueFamily = lveDevice.findPhysicalQueueFamilies().presentFamily;
   info.Queue = lveDevice.presentQueue();
   info.PipelineCache = VK_NULL_HANDLE;
   info.DescriptorPool = imguiPool;
   info.Subpass = 0;
   info.MinImageCount = lveRenderer.getSwapChainImageCount();
   info.ImageCount = lveRenderer.getSwapChainImageCount();
   info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
   info.Allocator = nullptr;
   info.CheckVkResultFn = check_vk_result;
   info.RenderPass = lveRenderer.getSwapChainRenderPass();

   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO &io = ImGui::GetIO();
   io.Fonts->AddFontDefault();
   ImGui_ImplGlfw_InitForVulkan(window, true);
   ImGui_ImplVulkan_Init(&info);
   ImGui_ImplVulkan_CreateFontsTexture();
   ImGui_ImplVulkan_DestroyFontsTexture();
   ImGui::StyleColorsDark();
}

void ImGuiGui::new_frame() {
   ImGui_ImplVulkan_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();
}

void ImGuiGui::update(lve::WaterMovementController &cameraControler,
                      bool &caminata, size_t &pipeline, glm::vec3 coord,
                      float frameTime, MyTextureData *img[],
                      SpectrumConfig params[], float &angle,
                      float (&colors)[3][4]) {
   ImGui::Begin("Sensibilidad");
   ImGui::SliderFloat("Velocidad minima", &cameraControler.moveSpeedMin,
                      0.1f, cameraControler.moveSpeedMax);
   ImGui::SliderFloat("Velocidad maxima", &cameraControler.moveSpeedMax,
                      cameraControler.moveSpeedMin, 500.f);
   ImGui::SliderFloat("Sensibilidad del mouse", &cameraControler.lookSpeed,
                      0.1f, 20.f);
   ImGui::Text("position: %f, %f, %f", coord.x, coord.y, coord.z);
   ImGui::Text("fps: %f, (%f ms)", 1 / frameTime, frameTime * 1000);
   ImGui::End();

   int caminata_i = caminata;
   ImGui::Begin("Modo de movimiento");
   ImGui::RadioButton("Caminata", &caminata_i, 1);
   ImGui::SameLine();
   ImGui::RadioButton("Vuelo", &caminata_i, 0);
   ImGui::End();
   caminata = caminata_i;

   int pipeline_i = pipeline;
   ImGui::Begin("Modo de rederizado");
   ImGui::RadioButton("Normal", &pipeline_i, 0);
   ImGui::SameLine();
   ImGui::RadioButton("WireFrame", &pipeline_i, 1);
   ImGui::SliderFloat("Sun angle", &angle, 3.14f, 6.3f);
   ImGui::End();
   pipeline = pipeline_i;

   ImGui::Begin("Init");
   ImGui::Image((ImTextureID)img[0]->DS,
                ImVec2(img[0]->Height, img[0]->Height));
   ImGui::End();

   ImGui::Begin("OnParamChange");
   ImGui::Image((ImTextureID)img[1]->DS,
                ImVec2(img[1]->Width, img[1]->Height));
   ImGui::Image((ImTextureID)img[2]->DS,
                ImVec2(img[2]->Width, img[2]->Height));
   ImGui::SameLine();
   ImGui::Image((ImTextureID)img[6]->DS,
                ImVec2(img[6]->Width, img[6]->Height));
   ImGui::Image((ImTextureID)img[3]->DS,
                ImVec2(img[3]->Width, img[3]->Height));
   ImGui::SameLine();
   ImGui::Image((ImTextureID)img[7]->DS,
                ImVec2(img[7]->Width, img[7]->Height));
   ImGui::Image((ImTextureID)img[4]->DS,
                ImVec2(img[4]->Width, img[4]->Height));
   ImGui::SameLine();
   ImGui::Image((ImTextureID)img[8]->DS,
                ImVec2(img[8]->Width, img[8]->Height));
   ImGui::Image((ImTextureID)img[5]->DS,
                ImVec2(img[5]->Width, img[5]->Height));
   ImGui::SameLine();
   ImGui::Image((ImTextureID)img[9]->DS,
                ImVec2(img[9]->Width, img[9]->Height));
   ImGui::End();

   ImGui::Begin("EveryFrame");
   ImGui::Image((ImTextureID)img[10]->DS,
                ImVec2(img[10]->Width, img[10]->Height));
   ImGui::SameLine();
   ImGui::Image((ImTextureID)img[11]->DS,
                ImVec2(img[11]->Width, img[11]->Height));
   ImGui::SameLine();
   ImGui::Image((ImTextureID)img[12]->DS,
                ImVec2(img[12]->Width, img[12]->Height));
   ImGui::SameLine();
   ImGui::Image((ImTextureID)img[13]->DS,
                ImVec2(img[13]->Width, img[13]->Height));
   ImGui::Image((ImTextureID)img[14]->DS,
                ImVec2(img[14]->Width, img[14]->Height));
   ImGui::SameLine();
   ImGui::Image((ImTextureID)img[15]->DS,
                ImVec2(img[15]->Width, img[15]->Height));
   ImGui::SameLine();
   ImGui::Image((ImTextureID)img[16]->DS,
                ImVec2(img[16]->Width, img[16]->Height));
   ImGui::SameLine();
   ImGui::Image((ImTextureID)img[17]->DS,
                ImVec2(img[17]->Width, img[17]->Height));
   ImGui::End();

   ImGui::Begin("Params waves");
   ImGui::SliderFloat("scale", &params[0].scale, 0.f, 1.f);
   ImGui::InputFloat("windSpeed", &params[0].windSpeed);
   ImGui::SliderAngle("windDirection", &params[0].windDirection, -180.f,
                      180.f);
   ImGui::InputFloat("fetch", &params[0].fetch);
   ImGui::SliderFloat("spreadBlend", &params[0].spreadBlend, 0.f, 1.f);
   ImGui::SliderFloat("swell", &params[0].swell, 0.f, 1.f);
   ImGui::InputFloat("peakEnhancement", &params[0].peakEnhancement);
   ImGui::InputFloat("shortWavesFade", &params[0].shortWavesFade);
   ImGui::End();

   ImGui::Begin("Params swell");
   ImGui::SliderFloat("scale", &params[1].scale, 0.f, 1.f);
   ImGui::InputFloat("windSpeed", &params[1].windSpeed);
   ImGui::SliderAngle("windDirection", &params[1].windDirection, -180.f,
                      180.f);
   ImGui::InputFloat("fetch", &params[1].fetch);
   ImGui::SliderFloat("spreadBlend", &params[1].spreadBlend, 0.f, 1.f);
   ImGui::SliderFloat("swell", &params[1].swell, 0.f, 1.f);
   ImGui::InputFloat("peakEnhancement", &params[1].peakEnhancement);
   ImGui::InputFloat("shortWavesFade", &params[1].shortWavesFade);
   ImGui::End();

   ImGui::Begin("Colors");
   ImGui::ColorPicker4("Sun", colors[0]);
   ImGui::ColorPicker3("Scatter", colors[1]);
   ImGui::ColorPicker3("Bubbles", colors[2]);
   ImGui::End();
}

void ImGuiGui::render(VkCommandBuffer command_buffer) {
   ImGui::Render();
   ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
}

ImGuiGui::~ImGuiGui() {
   ImGui_ImplVulkan_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

const char *vk_result_to_c_string(VkResult result) {
   switch (result) {
      case VK_SUCCESS:
         return "VK_SUCCESS";
      case VK_NOT_READY:
         return "VK_NOT_READY";
      case VK_TIMEOUT:
         return "VK_TIMEOUT";
      case VK_EVENT_SET:
         return "VK_EVENT_SET";
      case VK_EVENT_RESET:
         return "VK_EVENT_RESET";
      case VK_INCOMPLETE:
         return "VK_INCOMPLETE";
      case VK_ERROR_OUT_OF_HOST_MEMORY:
         return "VK_ERROR_OUT_OF_HOST_MEMORY";
      case VK_ERROR_OUT_OF_DEVICE_MEMORY:
         return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
      case VK_ERROR_INITIALIZATION_FAILED:
         return "VK_ERROR_INITIALIZATION_FAILED";
      case VK_ERROR_DEVICE_LOST:
         return "VK_ERROR_DEVICE_LOST";
      case VK_ERROR_MEMORY_MAP_FAILED:
         return "VK_ERROR_MEMORY_MAP_FAILED";
      case VK_ERROR_LAYER_NOT_PRESENT:
         return "VK_ERROR_LAYER_NOT_PRESENT";
      case VK_ERROR_EXTENSION_NOT_PRESENT:
         return "VK_ERROR_EXTENSION_NOT_PRESENT";
      case VK_ERROR_FEATURE_NOT_PRESENT:
         return "VK_ERROR_FEATURE_NOT_PRESENT";
      case VK_ERROR_INCOMPATIBLE_DRIVER:
         return "VK_ERROR_INCOMPATIBLE_DRIVER";
      case VK_ERROR_TOO_MANY_OBJECTS:
         return "VK_ERROR_TOO_MANY_OBJECTS";
      case VK_ERROR_FORMAT_NOT_SUPPORTED:
         return "VK_ERROR_FORMAT_NOT_SUPPORTED";
      case VK_ERROR_FRAGMENTED_POOL:
         return "VK_ERROR_FRAGMENTED_POOL";
      case VK_ERROR_UNKNOWN:
         return "VK_ERROR_UNKNOWN";
      case VK_ERROR_OUT_OF_POOL_MEMORY:
         return "VK_ERROR_OUT_OF_POOL_MEMORY";
      case VK_ERROR_INVALID_EXTERNAL_HANDLE:
         return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
      case VK_ERROR_FRAGMENTATION:
         return "VK_ERROR_FRAGMENTATION";
      case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
         return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
      case VK_PIPELINE_COMPILE_REQUIRED:
         return "VK_PIPELINE_COMPILE_REQUIRED";
      case VK_ERROR_SURFACE_LOST_KHR:
         return "VK_ERROR_SURFACE_LOST_KHR";
      case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
         return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
      case VK_SUBOPTIMAL_KHR:
         return "VK_SUBOPTIMAL_KHR";
      case VK_ERROR_OUT_OF_DATE_KHR:
         return "VK_ERROR_OUT_OF_DATE_KHR";
      case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
         return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
      case VK_ERROR_VALIDATION_FAILED_EXT:
         return "VK_ERROR_VALIDATION_FAILED_EXT";
      case VK_ERROR_INVALID_SHADER_NV:
         return "VK_ERROR_INVALID_SHADER_NV";
#ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
         return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
         return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
         return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
         return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
         return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
         return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
#endif
      case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
         return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
      case VK_ERROR_NOT_PERMITTED_KHR:
         return "VK_ERROR_NOT_PERMITTED_KHR";
      case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
         return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
      case VK_THREAD_IDLE_KHR:
         return "VK_THREAD_IDLE_KHR";
      case VK_THREAD_DONE_KHR:
         return "VK_THREAD_DONE_KHR";
      case VK_OPERATION_DEFERRED_KHR:
         return "VK_OPERATION_DEFERRED_KHR";
      case VK_OPERATION_NOT_DEFERRED_KHR:
         return "VK_OPERATION_NOT_DEFERRED_KHR";
      case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
         return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
      case VK_RESULT_MAX_ENUM:
         return "VK_RESULT_MAX_ENUM";
      default:
         return "??????";
   }
}
