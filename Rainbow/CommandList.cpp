﻿#include "CommandList.h"
#include "Renderer.h"
#include "RenderResource.h"

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
    render_pass_info.renderArea.extent = Renderer->GetViewportSize();

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

FVertexBuffer* FCommandList::CreateVertexBuffer(std::vector<FStaticVertex> VertexData, std::vector<uint32_t> IndicesData)
{
    FVertexBuffer* VertexBuffer = new FVertexBuffer();
    VertexBuffer->VertexBufferSize = VertexData.size();
    const size_t VertexBufferSize = sizeof(FStaticVertex) * VertexData.size();

    // Create a staging buffer to copy vertex data to the GPU
    VkBuffer StagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    CreateBuffer(VertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 StagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(Renderer->GetDevice(), stagingBufferMemory, 0, VertexBufferSize, 0, &data);
    memcpy(data, VertexData.data(), static_cast<size_t>(VertexBufferSize));
    vkUnmapMemory(Renderer->GetDevice(), stagingBufferMemory);

    CreateBuffer(VertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertexBuffer->VertexBuffer, VertexBuffer->VertexMemory);

    // Copy data from staging buffer to vertex buffer
    CopyBuffer(Renderer->GetCommandPool(), Renderer->GetGraphicsQueue(), StagingBuffer, VertexBuffer->VertexBuffer, VertexBufferSize);

    vkDestroyBuffer(Renderer->GetDevice(), StagingBuffer, nullptr);
    vkFreeMemory(Renderer->GetDevice(), stagingBufferMemory, nullptr);

    // Create Index Buffer
    VertexBuffer->IndexBufferSize = IndicesData.size();
    const size_t IndexBufferSize = sizeof(uint32_t) * IndicesData.size();
    CreateBuffer(IndexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 StagingBuffer, stagingBufferMemory);

    void* indexData;
    vkMapMemory(Renderer->GetDevice(), stagingBufferMemory, 0, IndexBufferSize, 0, &indexData);
    memcpy(indexData, IndicesData.data(), (size_t)IndexBufferSize);
    vkUnmapMemory(Renderer->GetDevice(), stagingBufferMemory);

    CreateBuffer(IndexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertexBuffer->IndexBuffer, VertexBuffer->IndexMemory);

    // Copy data from staging buffer to index buffer
    CopyBuffer(Renderer->GetCommandPool(), Renderer->GetGraphicsQueue(), StagingBuffer, VertexBuffer->IndexBuffer, IndexBufferSize);

    vkDestroyBuffer(Renderer->GetDevice(), StagingBuffer, nullptr);
    vkFreeMemory(Renderer->GetDevice(), stagingBufferMemory, nullptr);

    LOG_Info("Generating vertex and index buffer...");
    return VertexBuffer;
}

FTexture FCommandList::CreateTexture(uint32_t Witdh, uint32_t Height, VkFormat Format, VkImageUsageFlagBits Usage)
{
	FTexture NewTexture;
    VkImageAspectFlags aspectMask = 0;
	VkImageLayout imageLayout;

	NewTexture.Format = Format;

	if (Usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (Usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (Format >= VK_FORMAT_D16_UNORM_S8_UINT)
				aspectMask |=VK_IMAGE_ASPECT_STENCIL_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	check(aspectMask > 0);

	VkImageCreateInfo ImageCreateInfo {};
	ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageCreateInfo.format = Format;
	ImageCreateInfo.extent.width = Witdh;
	ImageCreateInfo.extent.height = Height;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.usage = Usage | VK_IMAGE_USAGE_SAMPLED_BIT;

	VkMemoryAllocateInfo MemoryAllocateInfo {};
	MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;

	if(vkCreateImage(Renderer->GetDevice(), &ImageCreateInfo, nullptr, &NewTexture.Image) != VK_SUCCESS)
	{
		checkf(0, "Unable to create VkImage");
	}
	
	vkGetImageMemoryRequirements(Renderer->GetDevice(), NewTexture.Image, &memReqs);
	MemoryAllocateInfo.allocationSize = memReqs.size;
	MemoryAllocateInfo.memoryTypeIndex = Renderer->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if(vkAllocateMemory(Renderer->GetDevice(), &MemoryAllocateInfo, nullptr, &NewTexture.ImageMemory) != VK_SUCCESS)
	{
		checkf(0, "Unable to allocated memory for VkImage");
	}

	if(vkBindImageMemory(Renderer->GetDevice(), NewTexture.Image, NewTexture.ImageMemory, 0) != VK_SUCCESS)
	{
		checkf(0, "Unable to bind memory for VkImage");
	}

	VkImageViewCreateInfo ImageViewCreateInfo {};
	ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	ImageViewCreateInfo.format = Format;
	ImageViewCreateInfo.subresourceRange = {};
	ImageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	ImageViewCreateInfo.subresourceRange.levelCount = 1;
	ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	ImageViewCreateInfo.subresourceRange.layerCount = 1;
	ImageViewCreateInfo.image = NewTexture.Image;

	if(vkCreateImageView(Renderer->GetDevice(), &ImageViewCreateInfo, nullptr, &NewTexture.ImageView) != VK_SUCCESS)
	{
		checkf(0, "Unable to create image view for VkImage");
	}
	
	return NewTexture;
}

bool FCommandList::GetSupportedDepthFormat(VkFormat* depthFormat)
{
	std::vector<VkFormat> formatList = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};

	for (const auto& format : formatList)
	{
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(Renderer->GetPhysicalDevice(), format, &formatProps);
		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			*depthFormat = format;
			return true;
		}
	}

	return false;
}

VkCommandBuffer FCommandList::CreateCommandBuffer(VkCommandBufferLevel CommandBufferLevel, bool begin)
{
	VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
	CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	CommandBufferAllocateInfo.commandPool = Renderer->GetCommandPool();
	CommandBufferAllocateInfo.level = CommandBufferLevel;
	CommandBufferAllocateInfo.commandBufferCount = 1;
	VkCommandBuffer CommandBuffer = nullptr;
	if(vkAllocateCommandBuffers(Renderer->GetDevice(), &CommandBufferAllocateInfo, &CommandBuffer) != VK_SUCCESS)
	{
		checkf(0, "Unable to allocate command buffer");
	}
	// If requested, also start recording for the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo) != VK_SUCCESS)
		{
			checkf(0, "Unable to create command buffer");
		}
	}
	return CommandBuffer;
}

VkDescriptorSetLayoutBinding FCommandList::DescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t descriptorCount)
{
	VkDescriptorSetLayoutBinding setLayoutBinding {};
	setLayoutBinding.descriptorType = type;
	setLayoutBinding.stageFlags = stageFlags;
	setLayoutBinding.binding = binding;
	setLayoutBinding.descriptorCount = descriptorCount;
	return setLayoutBinding;
}

void FCommandList::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Renderer->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        checkf(0, "Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Renderer->GetDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FRenderer::FindMemoryType(Renderer->GetPhysicalDevice(), memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(Renderer->GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        checkf(0, "Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(Renderer->GetDevice(), buffer, bufferMemory, 0);
}

void FCommandList::CopyBuffer(VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(Renderer->GetDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(Renderer->GetDevice(), commandPool, 1, &commandBuffer);
}
