#include "CommandList.h"
#include "Renderer.h"

FCommandList::FCommandList()
{
}

FCommandList::FCommandList(FRenderer* InRenderer)
{
    Renderer = InRenderer;
    FrameIndex = 0;
    LOG_Info("Creating command list");
}

void FCommandList::AcquireNextImage()
{
    check(Renderer);
    vkAcquireNextImageKHR(Renderer->GetDevice(),
        Renderer->GetSwapChain(),
        UINT64_MAX,
        Renderer->GetImageAvailableSemaphore(),
        VK_NULL_HANDLE,
        &FrameIndex);

    vkWaitForFences(Renderer->GetDevice(), 1, &Renderer->GetFences()[FrameIndex], VK_FALSE, UINT64_MAX);
    vkResetFences(Renderer->GetDevice(), 1, &Renderer->GetFences()[FrameIndex]);  

    CommandBuffer = Renderer->GetCommandBuffers()[FrameIndex];
    Image = Renderer->GetSwapChainImages()[FrameIndex];
}

void FCommandList::ResetCommandBuffer()
{
    vkResetCommandBuffer(CommandBuffer, 0);
}

void FCommandList::BeginCommandBuffer()
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(CommandBuffer, &beginInfo);
}

void FCommandList::EndCommandBuffer()
{
    vkEndCommandBuffer(CommandBuffer);
}

void FCommandList::FreeCommandBuffers()
{
    check(Renderer);
    vkFreeCommandBuffers(Renderer->GetDevice(), Renderer->GetCommandPool(), 1, &CommandBuffer);
}

void FCommandList::BeginRenderPass(VkClearColorValue ClearColorValue, VkClearDepthStencilValue ClearDepthStencilValue)
{
    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass        = Renderer->GetRenderPass();
    render_pass_info.framebuffer       = Renderer->GetSwapChainFrameBuffers()[FrameIndex];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = Renderer->GetSwapChainSize();

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = ClearColorValue;
    clearValues[1].depthStencil = ClearDepthStencilValue;

    render_pass_info.clearValueCount = static_cast<uint32_t>(clearValues.size());
    render_pass_info.pClearValues = clearValues.data();
	
    vkCmdBeginRenderPass(CommandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}

void FCommandList::EndRenderPass()
{
    vkCmdEndRenderPass(CommandBuffer);
}

VkPipelineStageFlags waitDestStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
void FCommandList::QueueSubmit()
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &Renderer->GetImageAvailableSemaphore();
    submitInfo.pWaitDstStageMask = &waitDestStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &CommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &Renderer->GetRenderingFinishedSemaphore();
    vkQueueSubmit(Renderer->GetGraphicsQueue(), 1, &submitInfo, Renderer->GetFences()[FrameIndex]);
}

void FCommandList::QueuePresent()
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &Renderer->GetRenderingFinishedSemaphore();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &Renderer->GetSwapChain();
    presentInfo.pImageIndices = &FrameIndex;
    vkQueuePresentKHR(Renderer->GetPresentQueue(), &presentInfo);

    vkQueueWaitIdle(Renderer->GetPresentQueue());
}

void FCommandList::SetViewport(int width, int height)
{
    VkViewport viewport;
    viewport.width = static_cast<float>(width / 2.f);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);
}

void FCommandList::SetScissor(int width, int height)
{
    VkRect2D scissor;
    scissor.extent.width = width / 2;
    scissor.extent.height = height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);
}
