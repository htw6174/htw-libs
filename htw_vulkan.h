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

typedef enum htw_BufferPoolType {
    HTW_BUFFER_POOL_TYPE_DIRECT = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // can be updated from host and used in pipelines with no transitions
} htw_BufferPoolType;

typedef enum htw_BufferUsageType {
    HTW_BUFFER_USAGE_VERTEX = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    HTW_BUFFER_USAGE_INDEX = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    HTW_BUFFER_USAGE_TEXTURE = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    HTW_BUFFER_USAGE_UNIFORM = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    HTW_BUFFER_USAGE_STORAGE = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
} htw_BufferUsageType;

// values correspond to descriptor set binding slots
typedef enum htw_DescriptorBindingFrequency {
    HTW_DESCRIPTOR_BINDING_FREQUENCY_PER_FRAME = 0,
    HTW_DESCRIPTOR_BINDING_FREQUENCY_PER_PASS = 1,
    HTW_DESCRIPTOR_BINDING_FREQUENCY_PER_PIPELINE = 2,
    HTW_DESCRIPTOR_BINDING_FREQUENCY_PER_OBJECT = 3,
} htw_DescriptorBindingFrequency;

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

typedef enum htw_VertexInputType {
    HTW_VERTEX_TYPE_UINT,
    HTW_VERTEX_TYPE_SINT,
    HTW_VERTEX_TYPE_FLOAT
} htw_VertexInputType;

typedef struct {
    uint32_t size;
    uint32_t offset;
    htw_VertexInputType inputType;
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

typedef VkDescriptorSetLayout htw_DescriptorSetLayout;

typedef VkDescriptorSet htw_DescriptorSet;

// TODO: update the pipeline/pipelinehandle format to match buffers (private _htw_pipeline, public htw_pipeline)
typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    size_t pushConstantSize;
    void *pushConstantData;
} htw_Pipeline;

typedef struct {
    VkBuffer buffer;
    VkMemoryRequirements deviceMemoryRequirements;
    VkDeviceSize deviceOffset;
} _htw_Buffer;

typedef _htw_Buffer* htw_Buffer;

typedef struct {
    htw_Buffer buffer;
    u32 subBufferCount;
    size_t subBufferHostSize;
    VkDeviceSize _subBufferDeviceSize;
} htw_SplitBuffer;

typedef struct {
    u32 maxCount;
    u32 currentCount;
    VkMemoryPropertyFlags memoryFlags;
    VkDeviceMemory deviceMemory;
    VkDeviceSize nextBufferMemoryOffset;
    _htw_Buffer *buffers;
} _htw_BufferPool;

typedef _htw_BufferPool* htw_BufferPool;

typedef struct {
    VkImage image;
    VkImageView view;
    VkSampler sampler;
    VkImageLayout layout;
    VkFormat format;
    VkDeviceMemory deviceMemory;
    u32 width;
    u32 height;
} htw_Texture;

typedef struct {
    htw_Buffer vertexBuffer;
    htw_Buffer indexBuffer;
    htw_Buffer instanceBuffer;
    u32 vertexCount;
    u32 indexCount;
    u32 instanceCount;
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

// TODO: hide this struct, provide an opaque pointer handle publically and use update methods to match
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

    uint32_t pipelineCount;
    htw_Pipeline *pipelines;

    VkDescriptorSetLayout defaultSetLayout;

    _htw_BufferPool bufferPool;

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

// descriptor set creation
// TODO: need to keep track of created layouts so they can be freed during teardown
htw_DescriptorSetLayout htw_createEmptySetLayout(htw_VkContext *vkContext);
htw_DescriptorSetLayout htw_createPerFrameSetLayout(htw_VkContext *vkContext);
htw_DescriptorSetLayout htw_createPerPassSetLayout(htw_VkContext *vkContext);
htw_DescriptorSetLayout htw_createPerPipelineSetLayout(htw_VkContext *vkContext);
htw_DescriptorSetLayout htw_createTextPipelineSetLayout(htw_VkContext *vkContext);
htw_DescriptorSetLayout htw_createPerObjectSetLayout(htw_VkContext *vkContext);
htw_DescriptorSetLayout htw_createTerrainObjectSetLayout(htw_VkContext *vkContext);
htw_DescriptorSet htw_allocateDescriptor(htw_VkContext *vkContext, htw_DescriptorSetLayout layout);
void htw_allocateDescriptors(htw_VkContext *vkContext, htw_DescriptorSetLayout layout, u32 count, htw_DescriptorSet *descriptorSets);
// associate descriptors with buffers. Requires buffers to be bound completely first
void htw_updatePerFrameDescriptor(htw_VkContext *vkContext, htw_DescriptorSet frameDescriptor, htw_Buffer windowInfo, htw_Buffer feedbackInfo, htw_Buffer worldInfo);
void htw_updatePerPassDescriptor(htw_VkContext *vkContext, htw_DescriptorSet passDescriptor); // TODO/UNUSED
void htw_updateTextDescriptor(htw_VkContext *vkContext, htw_DescriptorSet pipelineDescriptor, htw_Buffer uniformBuffer, htw_Texture glyphTexture);
void htw_updateTerrainPipelineDescriptor(htw_VkContext *vkContext, htw_DescriptorSet pipelineDescriptor); // TODO/UNUSED
void htw_updateTerrainObjectDescriptors(htw_VkContext *vkContext, htw_DescriptorSet* objectDescriptors, htw_SplitBuffer chunkBuffer);

/**
 * @brief returns a handle to a pipeline object that can be used in htw_bindDescriptor and htw_drawPipeline
 *
 * @param vkContext p_vkContext:...
 * @param layouts pointer to an array of at least 4 descriptor set layouts returned from an htw_create*SetLayout method (only the first 4 elements will be used)
 * @param shaderInfo p_shaderInfo:...
 * @return htw_PipelineHandle
 */
htw_PipelineHandle htw_createPipeline(htw_VkContext *vkContext, htw_DescriptorSetLayout *layouts, htw_ShaderSet shaderInfo);

htw_BufferPool htw_createBufferPool(htw_VkContext *vkContext, u32 poolItemCount, htw_BufferPoolType poolType);
htw_Buffer htw_createBuffer(htw_VkContext *vkContext, htw_BufferPool pool, size_t size, htw_BufferUsageType bufferType);
// Create a buffer that will be used as multiple smaller 'logical' buffers; the total size will be adjusted to account for gpu alignment requirements when binding each sub buffer, and *subBufferDeviceSize is set to device size per sub buffer
htw_SplitBuffer htw_createSplitBuffer(htw_VkContext *vkContext, htw_BufferPool pool, size_t subBufferSize, u32 subBufferCount, htw_BufferUsageType bufferType);
// Allocate enough GPU memory for all provided buffers, and bind pool buffers to that memory block
void htw_finalizeBufferPool(htw_VkContext *vkContext, htw_BufferPool pool);
// Write to the device memory backing a buffer
void htw_writeBuffer(htw_VkContext *vkContext, htw_Buffer buffer, void *hostData, size_t range);
void htw_writeSubBuffer(htw_VkContext *vkContext, htw_SplitBuffer *buffer, u32 subBufferIndex, void *hostData, size_t range);
// Pull device-side buffer contents to host accessible memory
void htw_retreiveBuffer(htw_VkContext *vkContext, htw_Buffer buffer, void *hostData, size_t range);
htw_Texture htw_createGlyphTexture(htw_VkContext *vkContext, uint32_t width, uint32_t height);
htw_Texture htw_createMappedTexture(htw_VkContext *vkContext, uint32_t width, uint32_t height);
// methods between begin and end one-time commands should only be called in between calls to the same
void htw_beginOneTimeCommands(htw_VkContext *vkContext);
void htw_updateTexture(htw_VkContext *vkContext, htw_Buffer source, htw_Texture dest);
void htw_endOneTimeCommands(htw_VkContext *vkContext);
// methods between begin and end frame should only be called in between calls to the same
void htw_beginFrame(htw_VkContext *vkContext);
void htw_bindPipeline(htw_VkContext *vkContext, htw_PipelineHandle pipelineHandle);
void htw_bindDescriptorSet(htw_VkContext *vkContext, htw_PipelineHandle pipelineHandle, htw_DescriptorSet descriptorSet, htw_DescriptorBindingFrequency bindFrequency);
void htw_pushConstants(htw_VkContext *vkContext, htw_PipelineHandle pipelineHandle, void *pushConstantData);
void htw_drawPipeline(htw_VkContext *vkContext, htw_PipelineHandle pipelineHandle, htw_ModelData *modelData, htw_DrawFlags drawFlags);
void htw_endFrame(htw_VkContext *vkContext);
void htw_resizeWindow(htw_VkContext *vkContext, int width, int height);
void htw_destroyVkContext(htw_VkContext *vkContext);

#endif // HTW_VULKAN_H_INCLUDED

