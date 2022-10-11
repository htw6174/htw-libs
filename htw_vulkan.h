#ifndef HTW_VULKAN_H_INCLUDED
#define HTW_VULKAN_H_INCLUDED

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "htw_core.h"

#ifdef VK_DEBUG
#define VK_CHECK(x) { VkResult test = x; \
                    if(test == VK_SUCCESS) printf("SUCCESS: %s\n", #x); \
                    else fprintf(stderr, "ERROR: %s failed with code %i\n", #x, test); }
#else
#define VK_CHECK(x) x
#endif

#define HTW_VK_WINDOW_FLAGS SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN
#define HTW_VK_MAX_SHADERS 100
#define HTW_VK_MAX_PIPELINES 100
#define HTW_VK_MAX_BUFFERS 100
#define HTW_SHADER_MAX_LENGTH 10000
#define HTW_MAX_AQUIRED_IMAGES 2

typedef uint32_t htw_ShaderHandle;
typedef uint32_t htw_ShaderLayoutHandle;
typedef uint32_t htw_PipelineHandle;

typedef enum htw_BufferUsageType {
    HTW_BUFFER_USAGE_VERTEX = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    HTW_BUFFER_USAGE_INDEX = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    HTW_BUFFER_USAGE_TEXTURE = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    HTW_BUFFER_USAGE_UNIFORM = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    HTW_BUFFER_USAGE_STORAGE = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
} htw_BufferUsageType;

typedef enum htw_DrawFlags {
    HTW_DRAW_TYPE_SIMPLE = 0, // vertex positions and tris generated by shader
    HTW_DRAW_TYPE_POINTS = 1, // vertex data only
    HTW_DRAW_TYPE_INDEXED = 3, // triangle indicies; vertex data also needed with indexed draw
    HTW_DRAW_TYPE_INSTANCED = 4 // instance data only
} htw_DrawFlags;

typedef enum htw_Samplers {
    HTW_SAMPLER_NONE = 0,
    HTW_SAMPLER_POINT,
    HTW_SAMPLER_BILINEAR,
    HTW_SAMPLER_ENUM_COUNT
} htw_Samplers;

typedef struct {
    uint32_t size;
    uint32_t offset;
} htw_ShaderInputInfo;

typedef struct {
    htw_ShaderHandle vertexShader;
    htw_ShaderHandle fragmentShader;
    uint32_t vertexInputStride;
    uint32_t vertexInputCount;
    htw_ShaderInputInfo *vertexInputInfos;
    uint32_t instanceInputStride;
    uint32_t instanceInputCount;
    htw_ShaderInputInfo *instanceInputInfos;
} htw_ShaderSet;

typedef struct {
    uint32_t pushConstantSize;
    uint32_t descriptorSetCount;
    VkDescriptorSetLayout *descriptorSetLayouts;
    VkDescriptorSet *descriptorSets;
    VkPipelineLayout pipelineLayout;
} htw_ShaderLayout;

typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    void *pushConstantData;
} htw_Pipeline;

typedef struct {
    VkBuffer buffer;
    size_t hostSize;
    void *hostData;
    VkMemoryRequirements deviceMemoryRequirements;
    VkDeviceSize deviceOffset;
} htw_Buffer;

typedef struct {
    VkImage image;
    VkImageView view;
    VkSampler sampler;
    VkImageLayout layout;
    VkFormat format;
    VkDeviceMemory deviceMemory;
    uint32_t width;
    uint32_t height;
} htw_Texture;

typedef struct {
    htw_Buffer *vertexBuffer;
    htw_Buffer *indexBuffer;
    htw_Buffer *instanceBuffer;
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t instanceCount;
    // texture data?
} htw_ModelData;

typedef struct {
    VkFormat format;
} htw_SwapchainInfo;

typedef struct {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFence queueSubmitFence; // copy of an aquiredImageFence
    VkSemaphore swapchainAquireSemaphore; // copy of an aquiredImageSemaphore
    VkSemaphore swapchainReleaseSemaphore; // one per SwapchainImageContext
} htw_SwapchainImageContext;

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
    VkCommandPool oneTimePool;
    VkCommandBuffer oneTimeBuffer;
    VkDescriptorPool descriptorPool;

    uint32_t shaderCount;
    VkShaderModule *shaders;
    uint32_t shaderLayoutCount;
    htw_ShaderLayout *shaderLayouts;

    uint32_t pipelineCount;
    htw_Pipeline *pipelines;

    uint32_t bufferCount;
    htw_Buffer *buffers;

    VkSampler *samplers;

    htw_Texture depthBuffer;
//     VkFormat depthImageFormat;
//     VkImageView depthImageView;
//     VkDeviceMemory depthImageMemory;


    VkRenderPass renderPass;

    VkSwapchainKHR swapchain;
    uint32_t swapchainImageCount;
    VkImageView *swapchainImageViews;
    VkFramebuffer *swapchainFramebuffers;
    htw_SwapchainImageContext *swapchainImages;
    htw_SwapchainInfo swapchainInfo;
    uint32_t aquiredImageCycleCounter;
    uint32_t currentImageIndex; // index into swapchainImages

    VkSemaphore *aquiredImageSemaphores;
    VkFence *aquiredImageFences;

    // device memory offest to address after last created buffer
    VkDeviceSize lastBufferOffset;
    VkDeviceMemory deviceMemory;
    //VkBuffer *uniformBuffers; // because uniforms are updated every frame, need to have as many uniform buffers as swapchain images

} htw_VkContext;

htw_VkContext *htw_createVkContext(SDL_Window *sdlWindow);
void htw_logHardwareProperties(htw_VkContext *vkContext);
/**
 * @brief Load a SPIR-V shader into a Vulkan shader module. The returned htw_ShaderHandle can be used to create rendering pipelines
 *
 * @param vkContext p_vkContext:...
 * @param filePath Path to a shader compiled into SPIR-V format. File extension should be .spv
 * @return htw_ShaderHandle
 */
htw_ShaderHandle htw_loadShader(htw_VkContext *vkContext, const char *filePath);
htw_ShaderLayout htw_createStandardShaderLayout (htw_VkContext *vkContext);
htw_ShaderLayout htw_createTextShaderLayout (htw_VkContext *vkContext);
htw_ShaderLayout htw_createTerrainShaderLayout (htw_VkContext *vkContext);
// associate descriptors with buffers. Requires buffers to be bound completely first
void htw_updateTextDescriptors(htw_VkContext *vkContext, htw_ShaderLayout shaderLayout, htw_Buffer uniformBuffer, htw_Texture glyphTexture);
void htw_updateTerrainDescriptors(htw_VkContext *vkContext, htw_ShaderLayout shaderLayout, htw_Buffer worldInfo, htw_Buffer terrainData);
htw_PipelineHandle htw_createPipeline(htw_VkContext *vkContext, htw_ShaderLayout shaderLayout, htw_ShaderSet shaderInfo);
htw_Buffer htw_createBuffer(htw_VkContext *vkContext, size_t size, htw_BufferUsageType bufferType);
// Allocate enough GPU memory for all provided buffers, and upload the contents of each
void htw_finalizeBuffers(htw_VkContext *vkContext, uint32_t bufferCount, htw_Buffer *buffers);
// bind buffers after allocation and before use
void htw_bindBuffers(htw_VkContext *vkContext, uint32_t count, htw_Buffer *buffers);
// Upload current buffer contents to the GPU
void htw_updateBuffer(htw_VkContext *vkContext, htw_Buffer *buffer);
void htw_updateBuffers(htw_VkContext *vkContext, uint32_t count, htw_Buffer *buffers);
htw_Texture htw_createGlyphTexture(htw_VkContext *vkContext, uint32_t width, uint32_t height);
htw_Texture htw_createMappedTexture(htw_VkContext *vkContext, uint32_t width, uint32_t height);
// methods between begin and end one-time commands should only be called in between calls to the same
void htw_beginOneTimeCommands(htw_VkContext *vkContext);
void htw_updateTexture(htw_VkContext *vkContext, htw_Buffer source, htw_Texture dest);
void htw_endOneTimeCommands(htw_VkContext *vkContext);
// NOTE: probably not a good way to do this because the caller could free before render loop is done
// Consider creating a staging void* with the same capacity, and having the user write to that.
void htw_mapPipelinePushConstant(htw_VkContext *vkContext, htw_PipelineHandle pipeline, void *pushConstantData);
// methods between begin and end frame should only be called in between calls to the same
void htw_beginFrame(htw_VkContext *vkContext);
void htw_drawPipeline(htw_VkContext *vkContext, htw_PipelineHandle pipelineHandle, htw_ShaderLayout *shaderLayout, htw_ModelData *modelData, htw_DrawFlags drawFlags);
void htw_endFrame(htw_VkContext *vkContext);
void htw_resizeWindow(htw_VkContext *vkContext, int width, int height);
void htw_destroyVkContext(htw_VkContext *vkContext);

#endif // HTW_VULKAN_H_INCLUDED

