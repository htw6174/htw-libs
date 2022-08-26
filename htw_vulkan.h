#ifndef HTW_VULKAN_H_INCLUDED
#define HTW_VULKAN_H_INCLUDED

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "htw_core.h"

#ifdef DEBUG
#define VK_CHECK(x) { VkResult test = x; \
                    if(test == VK_SUCCESS) printf("SUCCESS: %s\n", #x); \
                    else fprintf(stderr, "ERROR: %s failed with code %i\n", #x, test); }
#else
#define VK_CHECK(x) x
#endif

#define HTW_VK_WINDOW_FLAGS SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN
#define HTW_SHADER_MAX_LENGTH 10000
#define HTW_MAX_AQUIRED_IMAGES 2

typedef uint32_t htw_ShaderHandle;
typedef uint32_t htw_PipelineHandle;

typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    void *pushConstantData;
} htw_Pipeline;

typedef struct {
    VkBuffer buffer;
    size_t hostSize;
    void *hostData;
    VkMemoryRequirements deviceMemory;
    VkDeviceSize deviceOffset;
} htw_Buffer;

typedef struct {
    VkFormat format;
} htw_SwapchainInfo;

typedef struct {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFence queueSubmitFence;
    VkSemaphore swapchainAquireSemaphore;
    VkSemaphore swapchainReleaseSemaphore;
} htw_SwapchainImageContext;

typedef struct {
    VkCommandBuffer cmd;
    uint32_t imageIndex;
} htw_Frame;

typedef struct {
    SDL_Window *window;

    uint32_t width;
    uint32_t height;

    VkSurfaceKHR surface;
    VkInstanceCreateInfo instanceInfo;
    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice device;
    int32_t graphicsQueueIndex;
    VkQueue queue;
    VkCommandBuffer primaryBuffer;
    VkFramebuffer *swapchainFramebuffers;
    VkRenderPass renderPass;
    uint32_t shaderCount;
    VkShaderModule *shaders;
    uint32_t pipelineCount;
    htw_Pipeline *pipelines;

    VkFormat depthImageFormat;
    VkImageView depthImageView;
    VkDeviceMemory depthImageMemory;

    VkSwapchainKHR swapchain;
    uint32_t swapchainImageCount;
    VkImageView *swapchainImageViews;
    htw_SwapchainImageContext *swapchainImages;
    htw_SwapchainInfo swapchainInfo;
    uint32_t aquiredImageCycleCounter;

    VkSemaphore *aquiredImageSemaphores;
    VkFence *aquiredImageFences;

    // device memory offest to address after last created buffer
    VkDeviceSize lastBufferOffset;
    VkDeviceMemory deviceMemory;
    VkBuffer *uniformBuffers; // because uniforms are updated every frame, need to have as many uniform buffers as swapchain images

} htw_VkContext;

htw_VkContext *htw_createVkContext(SDL_Window *sdlWindow);
/**
 * @brief Load a SPIR-V shader into a Vulkan shader module. The returned htw_ShaderHandle can be used to create rendering pipelines
 *
 * @param vkContext p_vkContext:...
 * @param filePath Path to a shader compiled into SPIR-V format. File extension should be .spv
 * @return htw_ShaderHandle
 */
htw_ShaderHandle htw_loadShader(htw_VkContext *vkContext, const char *filePath);
htw_PipelineHandle htw_createPipeline(htw_VkContext *vkContext, htw_ShaderHandle vertShader, htw_ShaderHandle fragShader);
htw_Buffer htw_createBuffer(htw_VkContext *vkContext, size_t size);
// Allocate enough GPU memory for all provided buffers, and upload the contents of each
void htw_finalizeBuffers(htw_VkContext *vkContext, uint32_t bufferCount, htw_Buffer *buffers);
// Upload current buffer contents to the GPU
void htw_updateBuffer(htw_VkContext *vkContext, htw_Buffer *buffer);
// NOTE: probably not a good way to do this because the caller could free before render loop is done
// Consider creating a staging void* with the same capacity, and having the user write to that.
void htw_mapPipelinePushConstant(htw_VkContext *vkContext, htw_PipelineHandle pipeline, void *pushConstantData);
htw_Frame htw_beginFrame(htw_VkContext *vkContext);
void htw_drawPipeline(htw_VkContext *vkContext, htw_Frame frame, htw_PipelineHandle pipelineHandle, htw_Buffer instanceBuffer, uint32_t verticiesPerInstance, uint32_t instanceCount);
void htw_endFrame(htw_VkContext *vkContext, htw_Frame frame);
void htw_resizeWindow(htw_VkContext *vkContext, int width, int height);
void htw_destroyVkContext(htw_VkContext *vkContext);

static uint32_t getBestMemoryTypeIndex(htw_VkContext *vkContext, uint32_t memoryTypeBits, VkMemoryPropertyFlags propertyFlags);

static VkShaderModule loadShaderModule(htw_VkContext *vkContext, const char *filePath);
static htw_Pipeline createPipeline(htw_VkContext *vkContext, VkShaderModule vertShader, VkShaderModule fragShader);
static VkDeviceSize createInstanceBuffer(htw_VkContext *vkContext, VkBuffer *buffer);

static VkResult validateExtensions(uint32_t requiredCount, const char **requiredExtensions, uint32_t deviceExtCount, VkExtensionProperties *deviceExtensions);
static void initDevice(htw_VkContext *vkContext);
static void initDepthBuffer(htw_VkContext *vkContext);
static void initSwapchainImageContext(htw_VkContext *vkContext, htw_SwapchainImageContext *imageContext);
static void initRenderPass(htw_VkContext *vkContext);
static void initFramebuffers(htw_VkContext *vkContext);
static void initSwapchain(htw_VkContext *vkContext, uint32_t maxAquiredImages);
static void initUniformBuffers(htw_VkContext *vkContext);
static VkResult aquireNextImage(htw_VkContext *vkContext, uint32_t *imageIndex);
VkResult presentSwapchainImage(htw_VkContext* vkContext, uint32_t index);

#endif // HTW_VULKAN_H_INCLUDED

