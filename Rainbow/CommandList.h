#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "RenderResource.h"
#include "MinimalCore.h"

class FRenderer;

class FCommandList
{
public:
    FCommandList();
    FCommandList(FRenderer* InRenderer);
    
    void AcquireNextImage();
    void ResetCommandBuffer();
    void BeginCommandBuffer();
    void EndCommandBuffer();
    void FreeCommandBuffers();

    void BeginRenderPass(VkClearColorValue ClearColorValue, VkClearDepthStencilValue ClearDepthStencilValue);
    void EndRenderPass();

    void QueueSubmit();
    void QueuePresent();

    void SetViewport(int width,int height);
    void SetScissor(int width,int height);
    
    FVertexBuffer* CreateVertexBuffer(std::vector<FStaticVertex> VertexData, std::vector<uint32_t> IndicesData);
    FTexture CreateTexture(uint32_t Witdh, uint32_t Height, VkFormat Format, VkImageUsageFlagBits Usage);

    // library
    bool GetSupportedDepthFormat(VkFormat * depthFormat);
    VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel CommandBufferLevel, bool begin);
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t descriptorCount = 1);

private:
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CopyBuffer(VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

private:
    FRenderer* Renderer;
    uint32_t FrameIndex;
    VkCommandBuffer CommandBuffer;
    VkImage Image;
};
