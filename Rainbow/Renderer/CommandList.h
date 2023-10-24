#pragma once
#include <vulkan/vulkan_core.h>
#include "Core/MinimalCore.h"

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

private:
    FRenderer* Renderer;
    uint32_t FrameIndex;
    VkCommandBuffer CommandBuffer;
    VkImage Image;
};
