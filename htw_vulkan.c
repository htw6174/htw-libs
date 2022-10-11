#include <stdio.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>
#include "htw_vulkan.h"

typedef enum LayoutTransitionType {
    HTW_LAYOUT_TRANSITION_INIT_TO_COPY = 0,
    HTW_LAYOUT_TRANSITION_COPY_TO_FRAGMENT,
} LayoutTransitionType;

typedef struct LayoutTransitionMaskSet {
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
} LayoutTransitionMaskSet;

static const LayoutTransitionMaskSet layoutTransitionMaskSets[] = {
    {
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT
    },
    {
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    }
};

static uint32_t getBestMemoryTypeIndex(htw_VkContext *vkContext, uint32_t memoryTypeBits, VkMemoryPropertyFlags propertyFlags);
size_t getAlignedBufferSize (htw_VkContext* vkContext, size_t size, VkDeviceSize alignment);

static VkShaderModule loadShaderModule(htw_VkContext *vkContext, const char *filePath);
static htw_Pipeline createPipeline(htw_VkContext *vkContext, htw_ShaderLayout shaderLayout, htw_ShaderSet shaderInfo);
static htw_Texture createImage(htw_VkContext* vkContext, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlagBits usage, VkImageAspectFlagBits aspectFlags, htw_Samplers sampler);
static VkSampler createSampler(htw_VkContext *vkContext);

static void transitionImageLayout(VkCommandBuffer commandBuffer, htw_Texture *imageTexture, VkImageLayout newLayout, LayoutTransitionType transitionType);

static VkResult validateExtensions(uint32_t requiredCount, const char **requiredExtensions, uint32_t deviceExtCount, VkExtensionProperties *deviceExtensions);
static void initDevice(htw_VkContext *vkContext);
static void initDescriptorPool(htw_VkContext *vkContext);
static void initDepthBuffer(htw_VkContext *vkContext);
static void initSwapchainImageContext(htw_VkContext *vkContext, htw_SwapchainImageContext *imageContext);
static void initRenderPass(htw_VkContext *vkContext);
static void initFramebuffers(htw_VkContext *vkContext);
static void initGlobalCommandPools(htw_VkContext *vkContext);
static void initSwapchain(htw_VkContext *vkContext, uint32_t maxAquiredImages);
//static void initUniformBuffers(htw_VkContext *vkContext);
static VkResult aquireNextImage(htw_VkContext *vkContext, uint32_t *imageIndex);
VkResult presentSwapchainImage(htw_VkContext* vkContext, uint32_t index);

htw_VkContext *htw_createVkContext(SDL_Window *sdlWindow) {
    htw_VkContext *context = malloc(sizeof(htw_VkContext));
    // get sdl window dimensions
    int width, height;
    SDL_GetWindowSize(sdlWindow, &width, &height);
    context->width = (uint32_t)width;
    context->height = (uint32_t)height;

    // get number of required extensions
    unsigned int extraExtensionCount = 2;
    // TODO: only enable debug extensions when building in debug
    const char *extraExtensions[] = {VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_KHR_DISPLAY_EXTENSION_NAME};
    // get number of extensions to load for SDL
    unsigned int sdlRequiredExtensionCount;
    SDL_Vulkan_GetInstanceExtensions(sdlWindow, &sdlRequiredExtensionCount, NULL);
    unsigned int requiredExtensionCount = extraExtensionCount + sdlRequiredExtensionCount;
    // load SDL's required extensions
    const char **extensionNames = malloc(sizeof(char*) * requiredExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(sdlWindow, &sdlRequiredExtensionCount, extensionNames);
    // add other required extensions
    for (int i = 0; i < extraExtensionCount; i++) {
        extensionNames[sdlRequiredExtensionCount + i] = extraExtensions[i];
    }

#ifdef VK_DEBUG
    // log extensions
    printf("Using instance extensions:\n");
    for (int i = 0; i < requiredExtensionCount; i++) {
        printf("- %s\n", *(extensionNames + i));
    }
#endif

    // enable layers for validation TODO: only in debug
    uint32_t supportedLayerCount;
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, NULL);
    VkLayerProperties layerProperties[supportedLayerCount];
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, layerProperties);

#ifdef VK_DEBUG
    uint32_t requestedLayerCount = 1;
    const char *layerNames[] = {"VK_LAYER_KHRONOS_validation"};
#else
    uint32_t requestedLayerCount = 0;
    const char *layerNames[] = {};
#endif
    // TODO: full comparison of requested and available layers

#ifdef VK_DEBUG
    // log layers
    printf("Supported layers:\n");
    for (int i = 0; i < supportedLayerCount; i++) {
        char *name = layerProperties[i].layerName;
        printf("- %s", name);
        for (int r = 0; r < requestedLayerCount; r++) {
            if (strcmp(name, layerNames[r]) == 0) printf(" (required)");
        }
        printf("\n");
    }
#endif

    // create instance
    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledExtensionCount = requiredExtensionCount,
        .ppEnabledExtensionNames = extensionNames,
        .enabledLayerCount = requestedLayerCount,
        .ppEnabledLayerNames = layerNames
    };
    VkInstance instance;
    VK_CHECK(vkCreateInstance(&instanceInfo, NULL, &instance));
    free(extensionNames);

    // create surface through SDL
    VkSurfaceKHR vkSurface;
    SDL_Vulkan_CreateSurface(sdlWindow, instance, &vkSurface);

    context->window = sdlWindow;
    context->surface = vkSurface;
    context->instanceInfo = instanceInfo;
    context->instance = instance;

    // find and store a physical device for rendering
    initDevice(context);

#ifdef VK_DEBUG
    // log device properties
    htw_logHardwareProperties(context);
#endif

    initDescriptorPool(context);

    // create samplers
    context->samplers = malloc(sizeof(VkSampler) * HTW_SAMPLER_ENUM_COUNT);
    context->samplers[HTW_SAMPLER_NONE] = VK_NULL_HANDLE;
    context->samplers[HTW_SAMPLER_POINT] = createSampler(context);
    context->samplers[HTW_SAMPLER_BILINEAR] = createSampler(context); // TODO: differentiate between point and bilinear samplers

    // init image for depth buffer
    initDepthBuffer(context);
    // setup a swapchain of images that can be retreived, rendered to, and presented
    // swapchain must be initialized as NULL_HANDLE here, because initSwapchain may be called later (e.g. on window resize)
    context->swapchain = VK_NULL_HANDLE;
    initSwapchain(context, HTW_MAX_AQUIRED_IMAGES);
    context->aquiredImageCycleCounter = 0;
    // init render pass, framebuffers
    initRenderPass(context);
    initFramebuffers(context);
    initGlobalCommandPools(context);
    // init shader and pipeline caches
    // TODO: make an actual dynamic shader+pipeline library
    context->shaderCount = 0;
    context->shaders = malloc(sizeof(VkShaderModule) * HTW_VK_MAX_SHADERS);
    context->shaderLayoutCount = 0;
    context->shaderLayouts = malloc(sizeof(htw_ShaderLayout) * HTW_VK_MAX_SHADERS);
    context->pipelineCount = 0;
    context->pipelines = malloc(sizeof(htw_Pipeline) * HTW_VK_MAX_PIPELINES);
    context->bufferCount = 0;
    context->buffers = malloc(sizeof(htw_Buffer) * HTW_VK_MAX_BUFFERS);

    context->lastBufferOffset = 0;

    // init semaphores for aquired images
    context->aquiredImageSemaphores = malloc(sizeof(VkSemaphore) * HTW_MAX_AQUIRED_IMAGES);
    // init fences to track status of aquired images
    context->aquiredImageFences = malloc(sizeof(VkFence) * HTW_MAX_AQUIRED_IMAGES);
    // TODO: why not just create them here?
    for (int i = 0; i < HTW_MAX_AQUIRED_IMAGES; i++) {
        context->aquiredImageSemaphores[i] = VK_NULL_HANDLE;
        context->aquiredImageFences[i] = VK_NULL_HANDLE;
    }

    return context;
}

void htw_logHardwareProperties(htw_VkContext *vkContext) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(vkContext->gpu, &deviceProperties);
    VkPhysicalDeviceLimits deviceLimits = deviceProperties.limits;
    printf("API Version: %u\n", deviceProperties.apiVersion);
    printf("Driver Version: %u\n", deviceProperties.driverVersion);
    printf("Device Type: %u\n", deviceProperties.deviceType);
    printf("Device Name: %s\n", deviceProperties.deviceName);
    printf("Max image size: %u\n", deviceLimits.maxImageDimension2D);
    printf("Max uniform buffer size: %u\n", deviceLimits.maxUniformBufferRange);
    printf("Min uniform buffer allignment: %lu\n", deviceLimits.minUniformBufferOffsetAlignment);
    printf("Max dynamic uniform buffers: %i\n", deviceLimits.maxDescriptorSetUniformBuffersDynamic);
    printf("Max storage buffer size: %u\n", deviceLimits.maxStorageBufferRange);
    printf("Min storage buffer allignment: %lu\n", deviceLimits.minStorageBufferOffsetAlignment);
    printf("Max push constant size: %u\n", deviceLimits.maxPushConstantsSize);
    printf("Max memory allocations: %u\n", deviceLimits.maxMemoryAllocationCount);
    // TODO: memory heap sizes and total memory size
}

void htw_resizeWindow(htw_VkContext *vkContext, int width, int height) {
    // TODO
}

void htw_beginOneTimeCommands(htw_VkContext *vkContext) {
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = vkContext->oneTimePool,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(vkContext->device, &allocateInfo, &vkContext->oneTimeBuffer);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(vkContext->oneTimeBuffer, &beginInfo);
}

void htw_endOneTimeCommands(htw_VkContext *vkContext) {
    vkEndCommandBuffer(vkContext->oneTimeBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkContext->oneTimeBuffer
    };
    vkQueueSubmit(vkContext->queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkContext->queue);

    vkFreeCommandBuffers(vkContext->device, vkContext->oneTimePool, 1, &vkContext->oneTimeBuffer);
}

void htw_beginFrame(htw_VkContext *vkContext) {
    // get image from swapchain
    uint32_t imageIndex;
    aquireNextImage(vkContext, &imageIndex);
    // get a framebuffer and command buffer
    htw_SwapchainImageContext *frameContext = &vkContext->swapchainImages[imageIndex];
    VkFramebuffer framebuffer = vkContext->swapchainFramebuffers[imageIndex];
    VkCommandBufferBeginInfo cmdInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    // specifies that this will only be submitted once before being recycled
    cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    // begin command recording
    vkBeginCommandBuffer(frameContext->commandBuffer, &cmdInfo);
    // set clear color for swapchain image, and depth clear values for depth buffer (order is same as attachment order)
    VkClearValue clearValues[2];
    VkClearColorValue clearColors = {{0.1f, 0.1f, 0.2f, 1.0f}};
    VkClearDepthStencilValue depthClear = {.depth = 1.0f, .stencil = 0};
    clearValues[0].color = clearColors;
    clearValues[1].depthStencil = depthClear;
    // start render pass
    VkRenderPassBeginInfo rpInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vkContext->renderPass,
        .framebuffer = framebuffer,
        .renderArea.extent.width = vkContext->width,
        .renderArea.extent.height = vkContext->height,
        .clearValueCount = 2,
        .pClearValues = clearValues
    };
    vkCmdBeginRenderPass(frameContext->commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkContext->currentImageIndex = imageIndex;
}

void htw_drawPipeline (htw_VkContext* vkContext, htw_PipelineHandle pipelineHandle, htw_ShaderLayout *shaderLayout, htw_ModelData *modelData, htw_DrawFlags drawFlags)
{
    // bind the graphics pipeline
    htw_Pipeline currentPipeline = vkContext->pipelines[pipelineHandle];
    VkCommandBuffer cmd = vkContext->swapchainImages[vkContext->currentImageIndex].commandBuffer;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline.pipeline);

    // setup viewport
    VkViewport viewport = {
        .width = vkContext->width,
        .height = vkContext->height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    // setup scissor
    VkRect2D scissor = {
        .extent.width = vkContext->width,
        .extent.height = vkContext->height
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // draw vertices
    if (shaderLayout != NULL) vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderLayout->pipelineLayout, 0, shaderLayout->descriptorSetCount, shaderLayout->descriptorSets, 0, NULL);
    VkDeviceSize offsets[] = {0};
    uint32_t instanceCount = 1;
    // push constants
    if (shaderLayout->pushConstantSize > 0)
        vkCmdPushConstants(cmd, currentPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, shaderLayout->pushConstantSize, currentPipeline.pushConstantData);

    if ((drawFlags & HTW_DRAW_TYPE_POINTS) == HTW_DRAW_TYPE_POINTS) {
        vkCmdBindVertexBuffers(cmd, 0, 1, &modelData->vertexBuffer->buffer, offsets);
    }
    if ((drawFlags & HTW_DRAW_TYPE_INSTANCED) == HTW_DRAW_TYPE_INSTANCED) {
        instanceCount = modelData->instanceCount;
        vkCmdBindVertexBuffers(cmd, 0, 1, &modelData->instanceBuffer->buffer, offsets);
    }
    if ((drawFlags & HTW_DRAW_TYPE_INDEXED) == HTW_DRAW_TYPE_INDEXED) {
        vkCmdBindIndexBuffer(cmd, modelData->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, modelData->indexCount, instanceCount, 0, 0, 0);
    }
    else {
        vkCmdDraw(cmd, modelData->vertexCount, instanceCount, 0, 0);
    }
}

void htw_endFrame(htw_VkContext *vkContext) {
    htw_SwapchainImageContext currentImage = vkContext->swapchainImages[vkContext->currentImageIndex];
    // end render pass
    vkCmdEndRenderPass(currentImage.commandBuffer);
    // complete command buffer
    vkEndCommandBuffer(currentImage.commandBuffer);

    // submit command buffer to queue (needs a release semaphore)
    VkPipelineStageFlags waitStage = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &currentImage.commandBuffer,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &currentImage.swapchainAquireSemaphore,
        .pWaitDstStageMask = &waitStage,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &currentImage.swapchainReleaseSemaphore
    };
    // NOTE: [pSignalSemaphores] are signaled as all commands in the same VkSubmitInfo are completed. [fence] is signaled when all submitted commands are completed (for a single submitInfo they should be signaled at more or less the same time)
    vkQueueSubmit(vkContext->queue, 1, &submitInfo, currentImage.queueSubmitFence);

    // present to screen
    presentSwapchainImage(vkContext, vkContext->currentImageIndex);
    // advance image cycle counter
    vkContext->aquiredImageCycleCounter = (vkContext->aquiredImageCycleCounter + 1) % HTW_MAX_AQUIRED_IMAGES;

    SDL_UpdateWindowSurface(vkContext->window);
}

void htw_destroyVkContext(htw_VkContext* vkContext) {
    // wait for current rendering to complete
    vkWaitForFences(vkContext->device, HTW_MAX_AQUIRED_IMAGES, vkContext->aquiredImageFences, VK_TRUE, UINT64_MAX);

    // vulkan teardown
    for (int i = 0; i < vkContext->pipelineCount; i++) {
        vkDestroyPipelineLayout(vkContext->device, vkContext->pipelines[i].pipelineLayout, NULL);
        vkDestroyPipeline(vkContext->device, vkContext->pipelines[i].pipeline, NULL);
    }
    free(vkContext->pipelines);

    for (int i = 0; i < vkContext->shaderLayoutCount; i++) {
        htw_ShaderLayout layout = vkContext->shaderLayouts[i];
        for (int s = 0; s < layout.descriptorSetCount; s++) {
            vkDestroyDescriptorSetLayout(vkContext->device, layout.descriptorSetLayouts[s], NULL);
        }
        free(layout.descriptorSetLayouts);
        free(layout.descriptorSets);
    }
    free(vkContext->shaderLayouts);

    for (int i = 0; i < vkContext->shaderCount; i++) {
        vkDestroyShaderModule(vkContext->device, vkContext->shaders[i], NULL);
    }
    free(vkContext->shaders);

    for (int i = 0; i < vkContext->bufferCount; i++) {
        vkDestroyBuffer(vkContext->device, vkContext->buffers[i].buffer, NULL);
    }

    // because the first sampler is just VK_NULL_HANDLE, skip destoying it
    for (int i = 1; i < HTW_SAMPLER_ENUM_COUNT; i++) {
        vkDestroySampler(vkContext->device, vkContext->samplers[i], NULL);
    }
    free(vkContext->samplers);

    vkDestroyCommandPool(vkContext->device, vkContext->oneTimePool, NULL);

    for (int i = 0; i < vkContext->swapchainImageCount; i++) {
        htw_SwapchainImageContext image = vkContext->swapchainImages[i];
        vkFreeCommandBuffers(vkContext->device, image.commandPool, 1,  &image.commandBuffer);
        vkDestroyCommandPool(vkContext->device, image.commandPool, NULL);
        vkDestroySemaphore(vkContext->device, image.swapchainReleaseSemaphore, NULL);
        vkDestroyImageView(vkContext->device, vkContext->swapchainImageViews[i], NULL);
        vkDestroyFramebuffer(vkContext->device, vkContext->swapchainFramebuffers[i], NULL);
        //vkDestroyBuffer(vkContext->device, vkContext->uniformBuffers[i], NULL);
    }
    free(vkContext->swapchainImageViews);
    free(vkContext->swapchainImages);
    free(vkContext->swapchainFramebuffers);

    for (int i = 0; i < HTW_MAX_AQUIRED_IMAGES; i++) {
        vkDestroySemaphore(vkContext->device, vkContext->aquiredImageSemaphores[i], NULL);
        vkDestroyFence(vkContext->device, vkContext->aquiredImageFences[i], NULL);
    }
    free(vkContext->aquiredImageSemaphores);
    free(vkContext->aquiredImageFences);

    vkDestroyImageView(vkContext->device, vkContext->depthBuffer.view, NULL);
    vkDestroyImage(vkContext->device, vkContext->depthBuffer.image, NULL);
    vkFreeMemory(vkContext->device, vkContext->depthBuffer.deviceMemory, NULL);

    vkFreeMemory(vkContext->device, vkContext->deviceMemory, NULL);
    vkDestroySwapchainKHR(vkContext->device, vkContext->swapchain, NULL);
    vkDestroyRenderPass(vkContext->device, vkContext->renderPass, NULL);
    vkDestroyDescriptorPool(vkContext->device, vkContext->descriptorPool, NULL);
    vkDestroyDevice(vkContext->device, NULL);

    vkDestroySurfaceKHR(vkContext->instance, vkContext->surface, NULL);
    vkDestroyInstance(vkContext->instance, NULL);
    SDL_DestroyWindow(vkContext->window);

    // free memory
    free(vkContext);
}

htw_ShaderHandle htw_loadShader(htw_VkContext *vkContext, const char *filePath) {
    htw_ShaderHandle nextHandle = vkContext->shaderCount++;
    vkContext->shaders[nextHandle] = loadShaderModule(vkContext, filePath);
    return nextHandle;
}

htw_ShaderLayout htw_createStandardShaderLayout(htw_VkContext *vkContext) {
    htw_ShaderLayoutHandle nextLayout = vkContext->shaderLayoutCount++;
    htw_ShaderLayout newLayout = {
        .descriptorSetCount = 1,
        .descriptorSetLayouts = malloc(sizeof(VkDescriptorSetLayout) * newLayout.descriptorSetCount), // TODO: better allocator
        .descriptorSets = malloc(sizeof(VkDescriptorSet) * newLayout.descriptorSetCount) // TODO: better allocator
    };

    // define shader uniform layout
    VkDescriptorSetLayoutBinding layoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 2,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &layoutBinding
    };

    VK_CHECK(vkCreateDescriptorSetLayout(vkContext->device, &descriptorLayoutInfo, NULL, newLayout.descriptorSetLayouts));

    VkPushConstantRange pvRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = 128
    };

    VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = newLayout.descriptorSetLayouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pvRange
    };
    VK_CHECK(vkCreatePipelineLayout(vkContext->device, &layoutInfo, NULL, &newLayout.pipelineLayout));
    vkContext->shaderLayouts[nextLayout] = newLayout;
    return newLayout;
}

htw_ShaderLayout htw_createTextShaderLayout(htw_VkContext *vkContext) {
    htw_ShaderLayoutHandle nextLayout = vkContext->shaderLayoutCount++;
    htw_ShaderLayout newLayout = {
        .pushConstantSize = 0,
        .descriptorSetCount = 1,
        .descriptorSetLayouts = malloc(sizeof(VkDescriptorSetLayout) * newLayout.descriptorSetCount), // TODO: better allocator
        .descriptorSets = malloc(sizeof(VkDescriptorSet) * newLayout.descriptorSetCount) // TODO: better allocator
    };

    // define shader uniform layout
    VkDescriptorSetLayoutBinding uniformBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };
    // define shader uniform layout
    VkDescriptorSetLayoutBinding textureBinding = {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .pImmutableSamplers = NULL,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutBinding layoutBindings[] = {uniformBinding, textureBinding};

    VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = layoutBindings
    };

    VK_CHECK(vkCreateDescriptorSetLayout(vkContext->device, &descriptorLayoutInfo, NULL, newLayout.descriptorSetLayouts));

    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vkContext->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = newLayout.descriptorSetLayouts
    };
    vkAllocateDescriptorSets(vkContext->device, &allocateInfo, newLayout.descriptorSets);

    VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = newLayout.descriptorSetLayouts,
        .pushConstantRangeCount = 0,
    };
    VK_CHECK(vkCreatePipelineLayout(vkContext->device, &layoutInfo, NULL, &newLayout.pipelineLayout));
    vkContext->shaderLayouts[nextLayout] = newLayout;
    return newLayout;
}

htw_ShaderLayout htw_createTerrainShaderLayout(htw_VkContext *vkContext) {
    htw_ShaderLayoutHandle nextLayout = vkContext->shaderLayoutCount++;
    htw_ShaderLayout newLayout = {
        .descriptorSetCount = 1,
        .descriptorSetLayouts = malloc(sizeof(VkDescriptorSetLayout) * newLayout.descriptorSetCount), // TODO: better allocator
        .descriptorSets = malloc(sizeof(VkDescriptorSet) * newLayout.descriptorSetCount) // TODO: better allocator
    };

    VkDescriptorSetLayoutBinding worldInfoBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };

    VkDescriptorSetLayoutBinding terrainDataBinding = {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };

    VkDescriptorSetLayoutBinding layoutBindings[] = {worldInfoBinding, terrainDataBinding};

    VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = layoutBindings
    };

    VK_CHECK(vkCreateDescriptorSetLayout(vkContext->device, &descriptorLayoutInfo, NULL, newLayout.descriptorSetLayouts));

    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vkContext->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = newLayout.descriptorSetLayouts
    };
    vkAllocateDescriptorSets(vkContext->device, &allocateInfo, newLayout.descriptorSets);

    VkPushConstantRange pvRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = 128
    };
    newLayout.pushConstantSize = pvRange.size;

    VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = newLayout.descriptorSetLayouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pvRange
    };
    VK_CHECK(vkCreatePipelineLayout(vkContext->device, &layoutInfo, NULL, &newLayout.pipelineLayout));
    vkContext->shaderLayouts[nextLayout] = newLayout;
    return newLayout;
}

void htw_updateTextDescriptors(htw_VkContext *vkContext, htw_ShaderLayout shaderLayout, htw_Buffer uniformBuffer, htw_Texture glyphTexture) {
    VkDescriptorBufferInfo uniformBufferInfo = {
        .buffer = uniformBuffer.buffer,
        .offset = 0,
        .range = uniformBuffer.hostSize
    };
    VkDescriptorImageInfo glyphImageInfo = {
        .sampler = glyphTexture.sampler,
        .imageView = glyphTexture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL // NOTE: can't use current layout because it hasn't been transitioned yet; maybe set final layout when creating image, and use that as a 'destination' when doing image transitions?
    };

    VkWriteDescriptorSet uniformWriteInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = shaderLayout.descriptorSets[0],
        .dstBinding = 0,
        .dstArrayElement = 0, // used only for descriptor arrays
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &uniformBufferInfo
    };
    VkWriteDescriptorSet bitmapWriteInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = shaderLayout.descriptorSets[0],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .pImageInfo = &glyphImageInfo
    };
    VkWriteDescriptorSet writeSets[] = {uniformWriteInfo, bitmapWriteInfo};
    vkUpdateDescriptorSets(vkContext->device, 2, writeSets, 0, NULL);
}

void htw_updateTerrainDescriptors(htw_VkContext *vkContext, htw_ShaderLayout shaderLayout, htw_Buffer worldInfo, htw_Buffer terrainData) {

    VkDescriptorBufferInfo worldBufferInfo = {
        .buffer = worldInfo.buffer,
        .offset = 0,
        .range = worldInfo.hostSize
    };
    VkDescriptorBufferInfo terrainBufferInfo = {
        .buffer = terrainData.buffer,
        .offset = 0,
        .range = terrainData.hostSize
    };

    VkWriteDescriptorSet worldWriteInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = shaderLayout.descriptorSets[0],
        .dstBinding = 0,
        .dstArrayElement = 0, // used only for descriptor arrays
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &worldBufferInfo
    };
    VkWriteDescriptorSet terrainWriteInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = shaderLayout.descriptorSets[0],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &terrainBufferInfo
    };
    VkWriteDescriptorSet writeSets[] = {worldWriteInfo, terrainWriteInfo};
    vkUpdateDescriptorSets(vkContext->device, 2, writeSets, 0, NULL);
}

htw_PipelineHandle htw_createPipeline(htw_VkContext *vkContext, htw_ShaderLayout shaderLayout, htw_ShaderSet shaderInfo) {
    htw_PipelineHandle nextHandle = vkContext->pipelineCount++;
    vkContext->pipelines[nextHandle] = createPipeline(vkContext, shaderLayout, shaderInfo);
    return nextHandle;
}

htw_Buffer htw_createBuffer(htw_VkContext *vkContext, size_t size, htw_BufferUsageType bufferType) {
    uint32_t nextBuffer = vkContext->bufferCount++;
    htw_Buffer newBuffer;
    // create vkBuffer
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = bufferType,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    vkCreateBuffer(vkContext->device, &bufferInfo, NULL, &newBuffer.buffer);
    newBuffer.hostSize = size;
    newBuffer.hostData = malloc(size);
    // get device memory requirements
    vkGetBufferMemoryRequirements(vkContext->device, newBuffer.buffer, &newBuffer.deviceMemoryRequirements);
    if (newBuffer.deviceMemoryRequirements.memoryTypeBits == 0)
        fprintf(stderr, "No suitable memory type for buffer of size %lu, type %i\n", size, bufferType);
    size_t alignedSize = getAlignedBufferSize(vkContext, size, newBuffer.deviceMemoryRequirements.alignment);
    // set memory offset and move tracker forward
    newBuffer.deviceMemoryRequirements.size = alignedSize;
    newBuffer.deviceOffset = vkContext->lastBufferOffset;
    //printf("created buffer %i type = %i, size = %lu, offset = %lu\n", nextBuffer, bufferType, alignedSize, newBuffer.deviceOffset);
    vkContext->lastBufferOffset += alignedSize; // TODO: need to do anything else to ensure that the start of the next buffer created is aligned correctly?
    vkContext->buffers[nextBuffer] = newBuffer;

    return newBuffer;
}

void htw_finalizeBuffers(htw_VkContext *vkContext, uint32_t bufferCount, htw_Buffer *buffers) {
    // TODO: go through this process for each buffer type users can request and allocate a block of GPU memory for each
    // determine common memory requirements for all buffers
    uint32_t memoryTypeBits = 0xffffffff;
    VkDeviceSize combinedSize = 0;
    for (int i = 0; i < bufferCount; i++) {
        //printf("suitable memory bits for buffer %i: 0x%.8x\n", i, buffers[i].deviceMemoryRequirements.memoryTypeBits);
        memoryTypeBits = buffers[i].deviceMemoryRequirements.memoryTypeBits & memoryTypeBits;
        combinedSize += buffers[i].deviceMemoryRequirements.size;
    }
    if (memoryTypeBits == 0) { // no common suitable memory type
        fprintf(stderr, "No memory type meets common buffer memory requirements\n");
        exit(1);
    }
    //establish program memory requirements; must be able to copy data into device memory
    VkMemoryPropertyFlags programRequiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    // determine type of memory to use
    uint32_t memoryTypeIndex = getBestMemoryTypeIndex(vkContext, memoryTypeBits, programRequiredFlags);
    // allocate device memory; batch allocations to do this as few times as possible
    printf("allocating %lu bytes of device memory with flags %u\n", combinedSize, programRequiredFlags);
    VkMemoryAllocateInfo memoryInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = combinedSize,
        .memoryTypeIndex = memoryTypeIndex
    };
    VK_CHECK(vkAllocateMemory(vkContext->device, &memoryInfo, NULL, &vkContext->deviceMemory));
}

void htw_bindBuffers(htw_VkContext *vkContext, uint32_t count, htw_Buffer *buffers) {
    for (int i = 0; i < count; i++) {
        vkBindBufferMemory(vkContext->device, buffers[i].buffer, vkContext->deviceMemory, buffers[i].deviceOffset);
    }
}

// TODO: calling this per-frame runs fine right now, may need a solution with better performance later
void htw_updateBuffer(htw_VkContext *vkContext, htw_Buffer *buffer) {
    // copy program data to buffer
    // note: data is not always immediately copied into buffer memory. In this case the problem is solved by the use of memory with the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit set
    // data transfer to the GPU is done later, but guaranteed to be complete before any VkQueueSubmit work is started
    void* dest;
    vkMapMemory(vkContext->device, vkContext->deviceMemory, buffer->deviceOffset, buffer->deviceMemoryRequirements.size, 0, &dest);
    memcpy(dest, buffer->hostData, buffer->hostSize);
    vkUnmapMemory(vkContext->device, vkContext->deviceMemory);
}

void htw_updateBuffers(htw_VkContext *vkContext, uint32_t count, htw_Buffer *buffers) {
    for (int i = 0; i < count; i++) {
        htw_updateBuffer(vkContext, &buffers[i]);
    }
}

htw_Texture htw_createGlyphTexture(htw_VkContext *vkContext, uint32_t width, uint32_t height) {
    return createImage(vkContext, width, height, VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, HTW_SAMPLER_BILINEAR);
}

htw_Texture htw_createMappedTexture(htw_VkContext *vkContext, uint32_t width, uint32_t height) {
    return createImage(vkContext, width, height, VK_FORMAT_R16G16B16A16_SSCALED, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, HTW_SAMPLER_POINT);
}

void htw_updateTexture(htw_VkContext *vkContext, htw_Buffer source, htw_Texture dest) {
    VkCommandBuffer cmd = vkContext->oneTimeBuffer;

    transitionImageLayout(cmd, &dest, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, HTW_LAYOUT_TRANSITION_INIT_TO_COPY);

    VkImageSubresourceLayers subresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1
    };
    VkBufferImageCopy imageCopy = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = subresource,
        .imageOffset = {0, 0, 0},
        .imageExtent = {dest.width, dest.height, 1}
    };
    vkCmdCopyBufferToImage(cmd, source.buffer, dest.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

    transitionImageLayout(cmd, &dest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, HTW_LAYOUT_TRANSITION_COPY_TO_FRAGMENT);
}

void htw_mapPipelinePushConstant(htw_VkContext *vkContext, htw_PipelineHandle pipeline, void *pushConstantData) {
    vkContext->pipelines[pipeline].pushConstantData = pushConstantData;
}

static uint32_t getBestMemoryTypeIndex(htw_VkContext *vkContext, uint32_t memoryTypeBits, VkMemoryPropertyFlags propertyFlags) {
    if (memoryTypeBits == 0) { // no suitable memory type
        fprintf(stderr, "No memory type meets type bits requirements\n");
        return -1;
    }
    // get available memory types and heaps TODO: cache this lookup
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vkContext->gpu, &memoryProperties);
    // determine type of memory to use
    uint32_t memoryTypeIndex = 0;
    VkBool32 typeFound = VK_FALSE;
    for (int i = 0; i < memoryProperties.memoryTypeCount; i++) {
        // every bit in VkMemoryRequirements.memoryTypeBits corresponds to an element of VkPhysicalDeviceMemoryProperties.memoryTypes
        // where if the [i]th bit is set, memoryTypes[i] meets the buffer's memory requirements
        // in addition, memory heaps with better performance are generally located earlier in the memoryTypes list, so taking the first match is usually best
        uint32_t doesMeetBufferRequirements = memoryTypeBits & (1 << i);
        uint32_t doesMeetProgramRequirements = (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags;
        if (doesMeetBufferRequirements && doesMeetProgramRequirements) {
            memoryTypeIndex = i;
            typeFound = VK_TRUE;
            break;
        }
    }
    if (typeFound == VK_FALSE) {
        fprintf(stderr, "No suitable device memory found for buffer\n");
    }
    return memoryTypeIndex;
}

size_t getAlignedBufferSize (htw_VkContext* vkContext, size_t size, VkDeviceSize alignment) {
    size_t alignedSize = size; // return same size if it fits cleanly
    int diff = size % alignment;
    if (diff != 0) {
        // if there is an uneven remainder, increase to the next multiple of alignment
        alignedSize = size + (alignment - diff);
    }
    return alignedSize;
}

static VkShaderModule loadShaderModule(htw_VkContext *vkContext, const char *filePath) {
    // load .spv
    FILE *fp = fopen(filePath, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s\n", filePath);
        exit(1);
    }
    uint32_t wordCount = 0; // number of 32 bit words in the SPIR-V binary
    static uint32_t contents[HTW_SHADER_MAX_LENGTH];
    uint32_t c = 0;
    // Get first 4 bytes, check against SPIR-V magic number to determine validity and endianness
    c = getc(fp);
    c = (c << 8) | getc(fp);
    c = (c << 8) | getc(fp);
    c = (c << 8) | getc(fp);
    wordCount++;
    if (c == 0x07230203) { // read in normal order
        contents[0] = c;
        while((c = getc(fp)) != EOF) {
            c = (c << 8) | getc(fp);
            c = (c << 8) | getc(fp);
            c = (c << 8) | getc(fp);
            contents[wordCount] = c;
            wordCount++;
            if (wordCount == HTW_SHADER_MAX_LENGTH) {
                fprintf(stderr, "Shader size exceeds max readable length. Max length: %i\n", HTW_SHADER_MAX_LENGTH);
                exit(1);
            }
        }
    }
    else if (c == 0x03022307) { // read each word in reverse order
        contents[0] = 0x07230203;
        while((c = getc(fp)) != EOF) {
            c = c | (getc(fp) << 8);
            c = c | (getc(fp) << 16);
            c = c | (getc(fp) << 24);
            contents[wordCount] = c;
            wordCount++;
            if (wordCount == HTW_SHADER_MAX_LENGTH) {
                fprintf(stderr, "Shader size exceeds max readable length. Max length: %i\n", HTW_SHADER_MAX_LENGTH);
                exit(1);
            }
        }

    }
    else { // file doesn't fit the SPIR-V format spec
        fprintf(stderr, "Invalid SPIR-V format: %s\n", filePath);
        fclose(fp);
        exit(1);
    }

    fclose(fp);

    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = wordCount * 4, // convert from word count to byte count
        .pCode = contents
    };
    VK_CHECK(vkCreateShaderModule(vkContext->device, &info, NULL, &shaderModule));
    return shaderModule;
}

static htw_Pipeline createPipeline(htw_VkContext *vkContext, htw_ShaderLayout shaderLayout, htw_ShaderSet shaderInfo) {
    htw_Pipeline newPipeline;
    newPipeline.pipelineLayout = shaderLayout.pipelineLayout;

    // bind vertex shader inputs
    // FIXME/TODO: this will not work for shaders that use both vertex and instance buffers
    VkVertexInputBindingDescription bindingDescriptions[16];
    VkVertexInputAttributeDescription attributeDescriptions[16];
    int d = 0; // tracks description index
    for (int i = 0; i < shaderInfo.vertexInputCount; i++, d++) {
        htw_ShaderInputInfo layoutInfo = shaderInfo.vertexInputInfos[i];
        VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = shaderInfo.vertexInputStride,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
        bindingDescriptions[d] = bindingDescription;
        // determine format to use
        VkFormat format;
        switch (layoutInfo.size) {
            case 4:
                format = VK_FORMAT_R32_SFLOAT;
                break;
            case 8:
                format = VK_FORMAT_R32G32_SFLOAT;
                break;
            case 12:
                format = VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case 16:
                format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            default:
                format = VK_FORMAT_UNDEFINED;
                fprintf(stderr, "Invalid vertex input attribute size: %u", layoutInfo.size);
                break;
        }
        VkVertexInputAttributeDescription attributeDescription = {
            .location = i, // refers to location set in shader input layout
            .binding = 0, // index into the array of bound vertex buffers. Must match corresponding binding description
            .format = format, // usually something between VK_FORMAT_R32_SFLOAT for float, and VK_FORMAT_R32G32B32A32_SFLOAT for vec4
            .offset = layoutInfo.offset // from start of vertex or instance element in bytes
        };
        attributeDescriptions[d] = attributeDescription;
    }
    for (int i = 0; i < shaderInfo.instanceInputCount; i++, d++) {
        htw_ShaderInputInfo layoutInfo = shaderInfo.instanceInputInfos[i];
        VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = shaderInfo.instanceInputStride,
            .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
        };
        bindingDescriptions[d] = bindingDescription;
        // determine format to use
        VkFormat format;
        switch (layoutInfo.size) {
            case 4:
                format = VK_FORMAT_R32_SFLOAT;
                break;
            case 8:
                format = VK_FORMAT_R32G32_SFLOAT;
                break;
            case 12:
                format = VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case 16:
                format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            default:
                format = VK_FORMAT_UNDEFINED;
                fprintf(stderr, "Invalid instance input attribute size: %u", layoutInfo.size);
                break;
        }
        VkVertexInputAttributeDescription attributeDescription = {
            .location = i, // refers to location set in shader input layout
            .binding = 0, // index into the array of bound vertex buffers. Must match corresponding binding description
            .format = format, // usually something between VK_FORMAT_R32_SFLOAT for float, and VK_FORMAT_R32G32B32A32_SFLOAT for vec4
            .offset = layoutInfo.offset // from start of vertex or instance element in bytes
        };
        attributeDescriptions[d] = attributeDescription;
    }
    VkPipelineVertexInputStateCreateInfo vertexInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = d,
        .pVertexBindingDescriptions = bindingDescriptions,
        .vertexAttributeDescriptionCount = d,
        .pVertexAttributeDescriptions = attributeDescriptions
    };

    // use simple triangle lists
    VkPipelineInputAssemblyStateCreateInfo assemblyInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    // cull back faces and draw triangles clockwise
    VkPipelineRasterizationStateCreateInfo rasterizationInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0
    };

    // write to all color channels
    // TODO: enable alpha blending https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Color-blending
    VkPipelineColorBlendAttachmentState blendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD
    };

    VkPipelineColorBlendStateCreateInfo blendInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blendAttachment
    };

    // no pointers provided because we use dynamic state for these
    VkPipelineViewportStateCreateInfo viewportInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    // TODO: add to later. For now, enables depth test+write only and ignores stencils
    VkPipelineDepthStencilStateCreateInfo depthInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS, // LESS -> lower depth = closer; overwrite higher depth values
        .depthBoundsTestEnable = VK_FALSE, // if TRUE, only keep depth values within specified range
        .stencilTestEnable = VK_FALSE, // disable for now. Needs a valid front and back stencil to enable
    };

    // TODO: add to later. for now disables multisampling
    VkPipelineMultisampleStateCreateInfo multisampleInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    // indicate data that is not part of the pipeline state object
    VkDynamicState dynamics[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamics
    };

    VkShaderModule vertShader = vkContext->shaders[shaderInfo.vertexShader];
    VkShaderModule fragShader = vkContext->shaders[shaderInfo.fragmentShader];
    VkPipelineShaderStageCreateInfo vertShaderInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShader,
        .pName = "main"
    };
    VkPipelineShaderStageCreateInfo fragShaderInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShader,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shaders[] = {vertShaderInfo, fragShaderInfo};

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaders,
        .pVertexInputState = &vertexInfo,
        .pInputAssemblyState = &assemblyInfo,
        .pRasterizationState = &rasterizationInfo,
        .pColorBlendState = &blendInfo,
        .pMultisampleState = &multisampleInfo,
        .pViewportState = &viewportInfo,
        .pDepthStencilState = &depthInfo,
        .pDynamicState = &dynamicInfo,
        .renderPass = vkContext->renderPass,
        .layout = newPipeline.pipelineLayout
    };

    VK_CHECK(vkCreateGraphicsPipelines(vkContext->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &newPipeline.pipeline));

    // TODO: destroy shader modules after creation
    return newPipeline;
}

static htw_Texture createImage(htw_VkContext *vkContext, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlagBits usage, VkImageAspectFlagBits aspectFlags, htw_Samplers sampler) {
    // TODO: keep track of created images, image views, and allocated memory. These should be destroyed before destroying the underlying device.
    htw_Texture newTexture = {
        .width = width,
        .height = height,
        .layout = VK_IMAGE_LAYOUT_UNDEFINED,
        .sampler = vkContext->samplers[sampler],
        .format = format,
    };

    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = newTexture.layout
    };
    vkCreateImage(vkContext->device, &imageInfo, NULL, &newTexture.image);

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(vkContext->device, newTexture.image, &memoryRequirements);
    VkMemoryAllocateInfo memoryInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = getBestMemoryTypeIndex(vkContext, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    // NOTE: because my use case has few textures, using seperate allocations is fine for now
    // TODO: methods to declare images before allocating device memory for all images at once
    vkAllocateMemory(vkContext->device, &memoryInfo, NULL, &newTexture.deviceMemory);
    vkBindImageMemory(vkContext->device, newTexture.image, newTexture.deviceMemory, 0);

    // create image view
    VkImageSubresourceRange subresource = {
        .levelCount = 1,
        .baseMipLevel = 0,
        .layerCount = 1,
        .baseArrayLayer = 0,
        .aspectMask = aspectFlags
    };
    VkComponentMapping mapping = {
        .r = VK_COMPONENT_SWIZZLE_R,
        .g = VK_COMPONENT_SWIZZLE_G,
        .b = VK_COMPONENT_SWIZZLE_B,
        .a = VK_COMPONENT_SWIZZLE_A
    };
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .image = newTexture.image,
        .subresourceRange = subresource,
        .components = mapping // identity swizzle is 0, so this can be left unassigned
    };
    VK_CHECK(vkCreateImageView(vkContext->device, &viewInfo, NULL, &newTexture.view));

    return newTexture;
}

static VkSampler createSampler(htw_VkContext *vkContext) {
    VkSamplerCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR, // both linear = bilinear filtering
        .minFilter = VK_FILTER_LINEAR, // both point = point filtering
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 4.0f, // TODO: ensure this is <= the device limit for anisotropy
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK, // NOTE: only needed with address mode clamped
        .unnormalizedCoordinates = VK_FALSE, // false = sample coordinates in [0, 1] range; true = sample coordinates in [0, width or height] range
        // compare refers to a depth map in the same image. Mostly used for shadows
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        // no mipmaps used for now
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f
    };
    VkSampler newSampler;
    VK_CHECK(vkCreateSampler(vkContext->device, &createInfo, NULL, &newSampler));
    return newSampler;
}

static void transitionImageLayout(VkCommandBuffer commandBuffer, htw_Texture *imageTexture, VkImageLayout newLayout, LayoutTransitionType transitionType) {
    LayoutTransitionMaskSet maskSet = layoutTransitionMaskSets[transitionType];

    VkImageSubresourceRange subresource = {
        .levelCount = 1,
        .baseMipLevel = 0,
        .layerCount = 1,
        .baseArrayLayer = 0,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
    };
    // this structure can  be used to transition image layouts and transfer queue family ownership when VK_SHARING_MODE_EXCLUSIVE is used
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = imageTexture->layout,
        .newLayout = newLayout,
        // should be set to ignored unless changing queue family index
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = imageTexture->image,
        .subresourceRange = subresource,
        // TODO
        .srcAccessMask = maskSet.srcAccessMask,
        .dstAccessMask = maskSet.dstAccessMask
    };
    vkCmdPipelineBarrier(commandBuffer, maskSet.srcStageMask, maskSet.dstStageMask, 0, 0, NULL, 0, NULL, 1, &barrier);
    imageTexture->layout = newLayout;
}

static VkResult validateExtensions ( uint32_t requiredCount, const char** requiredExtensions, uint32_t deviceExtCount, VkExtensionProperties* deviceExtensions )
{
    for (int i = 0; i < requiredCount; i++) {
        int found = 0;
        for (int j = 0; j < deviceExtCount; j++) {
            if (strcmp(requiredExtensions[i], deviceExtensions[j].extensionName) == 0)
                found = 1;
        }
        if (found == 0) {
            printf("Required extension not found: %s\n", requiredExtensions[i]);
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    return VK_SUCCESS;
}

void initDevice(htw_VkContext *vkContext) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkContext->instance, &deviceCount, NULL); // get number of physical devices
    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(vkContext->instance, &deviceCount, devices); // get physical device data

    VkPhysicalDeviceFeatures requiredFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };

    // TODO: use required features to find the best physical device to use / check compatability

    vkContext->graphicsQueueIndex = -1;
    for (int i = 0; i < deviceCount; i++) {
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, NULL); // get number of queue families
        VkQueueFamilyProperties queueFamilies[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies); // get queue family properties

        // find a queue family that supports presenting to a surface
        for (int q = 0; q < queueFamilyCount; q++) {
            VkBool32 presentSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], q, vkContext->surface, &presentSupported);
            if ((queueFamilies[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupported) {
                vkContext->graphicsQueueIndex = q;
                break;
            }
        }

        // if a compatible queue family was found, use the current gpu
        if (vkContext->graphicsQueueIndex > -1) {
            vkContext->gpu = devices[i];
            break;
        }
    }

    uint32_t requiredExtensionCount = 1;
    const char *requiredExtensions[] = {"VK_KHR_swapchain"}; // TODO: make parameter, maybe store in window context

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(vkContext->gpu, NULL, &extensionCount, NULL);
    VkExtensionProperties extensionProperties[extensionCount];
    vkEnumerateDeviceExtensionProperties(vkContext->gpu, NULL, &extensionCount, extensionProperties);

    validateExtensions(
        requiredExtensionCount,
        requiredExtensions,
        extensionCount,
        extensionProperties);

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vkContext->graphicsQueueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
    VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        // TODO: any reason to set the enabled layer members here?
        .enabledExtensionCount = requiredExtensionCount,
        .ppEnabledExtensionNames = requiredExtensions,
        .pEnabledFeatures = &requiredFeatures
    };
    VK_CHECK(vkCreateDevice(vkContext->gpu, &deviceInfo, NULL, &vkContext->device));

    // TODO: reference code has a call to a 600 line method here. Is any of what it's doing needed?
    vkGetDeviceQueue(vkContext->device, vkContext->graphicsQueueIndex, 0, &vkContext->queue);
}

static void initDescriptorPool(htw_VkContext *vkContext) {
    // TODO
    uint32_t poolTypeCount = 3;
    VkDescriptorType poolTypes[] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
    VkDescriptorPoolSize poolSizes[3];
    for (int i = 0; i < poolTypeCount; i++) {
        VkDescriptorPoolSize poolSize = {
            .type = poolTypes[i],
            .descriptorCount = 100
        };
        poolSizes[i] = poolSize;
    }
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 100,
        .poolSizeCount = poolTypeCount,
        .pPoolSizes = poolSizes
    };
    VK_CHECK(vkCreateDescriptorPool(vkContext->device, &poolInfo, NULL, &vkContext->descriptorPool));
}

static void initDepthBuffer(htw_VkContext *vkContext) {
    // TODO: find device compatible format
    vkContext->depthBuffer = createImage(vkContext, vkContext->width, vkContext->height, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, HTW_SAMPLER_NONE);
}

void initSwapchainImageContext(htw_VkContext *vkContext, htw_SwapchainImageContext *imageContext){
    imageContext->queueSubmitFence = VK_NULL_HANDLE;
    imageContext->swapchainAquireSemaphore = VK_NULL_HANDLE;

    VkSemaphoreCreateInfo releaseSemaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK_CHECK(vkCreateSemaphore(vkContext->device, &releaseSemaphoreInfo, NULL, &imageContext->swapchainReleaseSemaphore));

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = vkContext->graphicsQueueIndex
    };
    VK_CHECK(vkCreateCommandPool(vkContext->device, &poolInfo, NULL, &imageContext->commandPool));

    VkCommandBufferAllocateInfo commandInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = imageContext->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VK_CHECK(vkAllocateCommandBuffers(vkContext->device, &commandInfo, &imageContext->commandBuffer));
}

void initSwapchain(htw_VkContext *vkContext, uint32_t maxAquiredImages) {
    VkSurfaceCapabilitiesKHR surfaceProperties;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkContext->gpu, vkContext->surface, &surfaceProperties));

    VkSurfaceFormatKHR format;
    uint32_t formatCount;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vkContext->gpu, vkContext->surface, &formatCount, NULL));
    VkSurfaceFormatKHR formats[formatCount];
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vkContext->gpu, vkContext->surface, &formatCount, formats));
    // TODO: ensure that the platform supports one of the application's required formats
    // for now, just assume that the first format is OK
    format = formats[0];
    // decent default format:
    // format.format = VK_FORMAT_R8G8B8A8_UNORM;

    // TODO: full of hardcoded values, figure out what all of these do
    VkSwapchainCreateInfoKHR swapchainInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkContext->surface,
        .minImageCount = surfaceProperties.minImageCount + (maxAquiredImages - 1), // allows application to have control of maxAquiredImages 'in-flight' frames
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent.width = vkContext->width,
        .imageExtent.height = vkContext->height,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE, // allows vulkan to skip rendering for pixels that aren't visible on screen. Generally leave this on.
        .oldSwapchain = vkContext->swapchain
    };
    VK_CHECK(vkCreateSwapchainKHR(vkContext->device, &swapchainInfo, NULL, &vkContext->swapchain));
    // TODO: clean up old swapchain if not null

    vkContext->swapchainInfo.format = format.format;

    // Setup as many images as the swapchain will use
    uint32_t imageCount;
    VK_CHECK(vkGetSwapchainImagesKHR(vkContext->device, vkContext->swapchain, &imageCount, NULL));
    vkContext->swapchainImageCount = imageCount;
    VkImage images[imageCount];
    VK_CHECK(vkGetSwapchainImagesKHR(vkContext->device, vkContext->swapchain, &imageCount, images));

    vkContext->swapchainImageViews = malloc(sizeof(VkImageView) * imageCount);
    for (int i = 0; i < imageCount; i++) {
        VkImageSubresourceRange subresource = {
            .levelCount = 1,
            .layerCount = 1,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        };
        VkComponentMapping mapping = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A
        };
        VkImageViewCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vkContext->swapchainInfo.format,
            .image = images[i],
            .subresourceRange = subresource,
            .components = mapping
        };
        VK_CHECK(vkCreateImageView(vkContext->device, &info, NULL, &vkContext->swapchainImageViews[i]));
    }

    vkContext->swapchainImages = malloc(sizeof(htw_SwapchainImageContext) * imageCount);
    for (int i = 0; i < imageCount; i++) {
        initSwapchainImageContext(vkContext, &vkContext->swapchainImages[i]);
    }
}

void initRenderPass(htw_VkContext *vkContext) {
    // TODO: what is an attachment?
    VkAttachmentDescription colorAttachment = {
        .flags = 0,
        .format = vkContext->swapchainInfo.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        // clear and store tiles
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    // NOTE: stencil bits in color format are unused for now, but likely useful later
    VkAttachmentDescription depthAttachment = {
        .flags = 0,
        .format = vkContext->depthBuffer.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference colorReference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    VkAttachmentReference depthReference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    VkSubpassDescription subpass = {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorReference,
        .pDepthStencilAttachment = &depthReference
    };

    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};

    // specifies what stuff needs to be available in each rendering step, e.g. because depth buffer is cleared on load, then (?)
    // TODO: figure out dependency details
    VkSubpassDependency dependency = {
        .dependencyFlags = 0,
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo rpInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    VK_CHECK(vkCreateRenderPass(vkContext->device, &rpInfo, NULL, &vkContext->renderPass));
}

void initFramebuffers(htw_VkContext *vkContext) {
    vkContext->swapchainFramebuffers = malloc(sizeof(VkFramebuffer) * vkContext->swapchainImageCount);

    for (int i = 0; i < vkContext->swapchainImageCount; i++) {
        VkImageView imageViews[] = {vkContext->swapchainImageViews[i], vkContext->depthBuffer.view};
        VkFramebufferCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vkContext->renderPass,
            .attachmentCount = 2,
            .pAttachments = imageViews,
            .width = vkContext->width,
            .height = vkContext->height,
            .layers = 1
        };
        VK_CHECK(vkCreateFramebuffer(vkContext->device, &info, NULL, &vkContext->swapchainFramebuffers[i]));
    }
}

static void initGlobalCommandPools(htw_VkContext *vkContext) {
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = vkContext->graphicsQueueIndex
    };
    VK_CHECK(vkCreateCommandPool(vkContext->device, &poolInfo, NULL, &vkContext->oneTimePool));
}

// static void initUniformBuffers(htw_VkContext *vkContext) {
//     vkContext->uniformBuffers = malloc(sizeof(VkBuffer) * vkContext->swapchainImageCount);
//
//     for (int i = 0; i < vkContext->swapchainImageCount; i++) {
//         VkBufferCreateInfo info = {
//             .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
//             .size = 0,
//             .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
//             .sharingMode = VK_SHARING_MODE_CONCURRENT,
//
//         };
//         VK_CHECK(vkCreateBuffer(vkContext->device, &info, NULL, &vkContext->uniformBuffers[i]));
//     }
// }

VkResult aquireNextImage(htw_VkContext *vkContext, uint32_t *imageIndex) {
    // create fence if not initialized
    VkFence *oldestFence = &vkContext->aquiredImageFences[vkContext->aquiredImageCycleCounter];
    if (*oldestFence == VK_NULL_HANDLE) {
        VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        VK_CHECK(vkCreateFence(vkContext->device, &fenceInfo, NULL, oldestFence));
    }

    // wait for oldest aquired image to finish rendering
    vkWaitForFences(vkContext->device, 1, oldestFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkContext->device, 1, oldestFence);

    VkSemaphore *aquireSemaphore = &vkContext->aquiredImageSemaphores[vkContext->aquiredImageCycleCounter];
    // create semaphore if not initialized
    if (*aquireSemaphore == VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo aquireSemaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(vkContext->device, &aquireSemaphoreInfo, NULL, aquireSemaphore));
    }

    // returns as soon as the index of the next presentable image can be aquired; hoewever the image may still be in use, and cannot be immediately written to. The provided fence or semaphore will be signaled when the returned image is fully available.
    VkResult res = vkAcquireNextImageKHR(vkContext->device, vkContext->swapchain, UINT64_MAX, *aquireSemaphore, VK_NULL_HANDLE, imageIndex);

    htw_SwapchainImageContext *ic = &vkContext->swapchainImages[*imageIndex];
    ic->swapchainAquireSemaphore = *aquireSemaphore;
    ic->queueSubmitFence = *oldestFence;

    vkResetCommandPool(vkContext->device, ic->commandPool, 0);

    return res;
}

VkResult presentSwapchainImage(htw_VkContext *vkContext, uint32_t index) {
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &vkContext->swapchain,
        .pImageIndices = &index,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkContext->swapchainImages[index].swapchainReleaseSemaphore
    };
    return vkQueuePresentKHR(vkContext->queue, &presentInfo);
}


