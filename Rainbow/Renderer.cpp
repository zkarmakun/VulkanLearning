#pragma once
#include "Renderer.h"

#include <array>
#include <glm/glm.hpp>
#include <set>
#include <SDL2/SDL_vulkan.h>
#include <vector>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_log.h>
#include <vulkan/vulkan_core.h>
#include "CommandList.h"
#include "RenderWindow.h"
#include "World.h"

FCommandList FRenderer::CmdList;

FRenderer::FRenderer()
{
    bInitialized = false;
    pRenderWindow = nullptr;
    World = nullptr;
}

void FRenderer::Init(FRenderWindow* RenderWindow)
{
    pRenderWindow = RenderWindow;
    CreateInstance();
    CreateDebug();
    CreateSurface();
    LOG_Info("Initializing vulkan instance");
    SelectPhysicalDevice();
    SelectQueueFamily();
    CreateDevice();
    CreateSwapChain();
    SetupDepthStencil();
    CreateRenderPass();
    CreateFrameBuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSemaphores();
    CreateFences();

    CmdList = FCommandList(this);
    CreateGBuffer();

    LOG_Info("Initializing vulkan completed");

    World = new FWorld();
    World->LoadWorld();
    bInitialized = true;
}

VkPipelineStageFlags PipelineStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
void FRenderer::RenderLoop()
{
    bool stillRunning = true;
    while(stillRunning) {

        SDL_Event event;
        while(SDL_PollEvent(&event)) {

            switch(event.type) {

            case SDL_QUIT:
                stillRunning = false;
                break;

            default:
                // Do nothing.
                break;
            }
        }
        
        
        GetCommandList().AcquireNextImage();

        GetCommandList().ResetCommandBuffer();
        GetCommandList().BeginCommandBuffer();
        {
            VkClearColorValue ClearColor = {0.2f, 1.f, 0.2f, 1.0f};
            VkClearDepthStencilValue ClearDepthStencilValue = {1.0f, 0};
            GetCommandList().BeginRenderPass(ClearColor, ClearDepthStencilValue);
            {
                World->Render();
            }
            GetCommandList().EndRenderPass();
        }
        GetCommandList().EndCommandBuffer();

        GetCommandList().QueueSubmit();
        GetCommandList().QueuePresent();
    }
}

void FRenderer::Shutdown()
{
    if(!bInitialized) return;
    
    vkDestroySurfaceKHR(Instance, SurfaceKHR, nullptr);
    vkDestroyInstance(Instance, nullptr);
}

VkImageView FRenderer::CreateImageView(VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = Format;
    viewInfo.subresourceRange.aspectMask = AspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
       check(0);
    }

    return imageView;
}

void FRenderer::CreateImage(uint32_t Width, uint32_t Height, VkFormat Format, VkImageTiling Tiling, VkImageUsageFlags ImageUsageFlags, VkMemoryPropertyFlags MemoryPropertyFlags, VkImage& Image, VkDeviceMemory& ImageMemory)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = Width;
    imageInfo.extent.height = Height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = Format;
    imageInfo.tiling = Tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = ImageUsageFlags;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(Device, &imageInfo, nullptr, &Image) != VK_SUCCESS)
    {
        check(0);
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(Device, Image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(PhysicalDevice, memRequirements.memoryTypeBits, MemoryPropertyFlags);

    if (vkAllocateMemory(Device, &allocInfo, nullptr, &ImageMemory) != VK_SUCCESS)
    {
        check(0);
    }

    vkBindImageMemory(Device, Image, ImageMemory, 0);
}

uint32_t FRenderer::FindMemoryType(const VkPhysicalDevice& PhysicalDevice, uint32_t TypeFilter, VkMemoryPropertyFlags MemoryPropertyFlags)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((TypeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & MemoryPropertyFlags) == MemoryPropertyFlags)
        {
            return i;
        }
    }

    return 0;
}

uint32_t FRenderer::GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound) const
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                if (memTypeFound)
                {
                    *memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }

    if (memTypeFound)
    {
        *memTypeFound = false;
        return 0;
    }
    checkf(0, "Could not find a matching memory type");
}

VkDevice& FRenderer::GetDevice()
{
    return Device;
}

VkPhysicalDevice& FRenderer::GetPhysicalDevice()
{
    return PhysicalDevice;
}

VkSwapchainKHR& FRenderer::GetSwapChain()
{
    return SwapChain;
}

VkSemaphore& FRenderer::GetImageAvailableSemaphore()
{
    return ImageAvailableSemaphore;
}

VkSemaphore& FRenderer::GetRenderingFinishedSemaphore()
{
    return RenderingFinishedSemaphore;
}

std::vector<VkFence>& FRenderer::GetFences()
{
    return Fences;
}

std::vector<VkCommandBuffer>& FRenderer::GetCommandBuffers()
{
    return CommandBuffers;
}

std::vector<VkImage>& FRenderer::GetSwapChainImages()
{
    return SwapChainImages;
}

VkCommandPool& FRenderer::GetCommandPool()
{
    return CommandPool;
}

VkRenderPass& FRenderer::GetRenderPass()
{
    return RenderPass;
}

std::vector<VkFramebuffer>& FRenderer::GetSwapChainFrameBuffers()
{
    return SwapChainFrameBuffers;
}

VkExtent2D& FRenderer::GetViewportSize()
{
    return ViewportSize;
}

VkQueue& FRenderer::GetGraphicsQueue()
{
    return GraphicsQueue;
}

VkQueue& FRenderer::GetPresentQueue()
{
    return PresentQueue;
}

FCommandList& FRenderer::GetCommandList()
{
    return CmdList;
}

void FRenderer::CreateInstance()
{
    if(!pRenderWindow)
    {
        checkf(0, "Failed creating vulkan instance, no window pointer");   
    }
    
    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(pRenderWindow->GetWindow(), &extensionCount, nullptr);
    vector<const char *> extensionNames(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(pRenderWindow->GetWindow(), &extensionCount, extensionNames.data());

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = pRenderWindow->GetWindowName().c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "MyEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount = validationLayers.size();
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    instanceCreateInfo.enabledExtensionCount = extensionNames.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensionNames.data();

    if(vkCreateInstance(&instanceCreateInfo, nullptr, &Instance) != VK_SUCCESS)
    {
        checkf(0, "Failed creating vulkan instance");  
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanReportFunc(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData)
{
    printf("VULKAN VALIDATION: %s\n", msg);
    return VK_FALSE;
}

PFN_vkCreateDebugReportCallbackEXT SDL2_vkCreateDebugReportCallbackEXT = nullptr;
void FRenderer::CreateDebug()
{
    SDL2_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)SDL_Vulkan_GetVkGetInstanceProcAddr();
    VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo = {};
    debugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debugCallbackCreateInfo.pfnCallback = VulkanReportFunc;

    SDL2_vkCreateDebugReportCallbackEXT(Instance, &debugCallbackCreateInfo, 0, &debugCallback);
}

void FRenderer::CreateSurface()
{
    if(SDL_Vulkan_CreateSurface(pRenderWindow->GetWindow(), Instance, &SurfaceKHR) != SDL_TRUE)
    {
        checkf(0, "Failed creating vulkan surface");
    }
}

void FRenderer::SelectPhysicalDevice()
{
    std::vector<VkPhysicalDevice> PhysicalDevices;
    uint32_t physicalDeviceCount = 0;

    vkEnumeratePhysicalDevices(Instance, &physicalDeviceCount, nullptr);
    PhysicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(Instance, &physicalDeviceCount, PhysicalDevices.data());

    if(!PhysicalDevices.empty())
    {
        PhysicalDevice = PhysicalDevices[0];
        for(const auto& Device : PhysicalDevices)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(Device, &deviceProperties);
            LOG_Info("Selected device [%s] Type: %i API: %i", deviceProperties.deviceName, static_cast<int>(deviceProperties.deviceType), deviceProperties.apiVersion);
        }
        return;
    }
    checkf(0, "Failed selecting device");
}

void FRenderer::SelectQueueFamily()
{
    vector<VkQueueFamilyProperties> queueFamilyProperties;
    uint32_t queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, nullptr);
    queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    int graphicIndex = -1;
    int presentIndex = -1;

    int i = 0;
    for(const auto& queueFamily : queueFamilyProperties)
    {
        if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicIndex = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, SurfaceKHR, &presentSupport);
        if(queueFamily.queueCount > 0 && presentSupport)
        {
            presentIndex = i;
        }

        if(graphicIndex != -1 && presentIndex != -1)
        {
            break;
        }

        i++;
    }

    graphics_QueueFamilyIndex = graphicIndex;
    present_QueueFamilyIndex = presentIndex;
}

void FRenderer::CreateDevice()
{
    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const float queue_priority[] = { 1.0f };

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { graphics_QueueFamilyIndex, present_QueueFamilyIndex };

    float queuePriority = queue_priority[0];
    for(int queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphics_QueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();

    vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &Device);

    vkGetDeviceQueue(Device, graphics_QueueFamilyIndex, 0, &GraphicsQueue);
    vkGetDeviceQueue(Device, present_QueueFamilyIndex, 0, &PresentQueue);
}

void FRenderer::CreateSwapChain()
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, SurfaceKHR, &SurfaceCapabilitiesKHR);
   
    vector<VkSurfaceFormatKHR> surfaceFormats;
    uint32_t surfaceFormatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, SurfaceKHR,
                                                           &surfaceFormatsCount,
                                                           nullptr);
    surfaceFormats.resize(surfaceFormatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, SurfaceKHR,
                                                           &surfaceFormatsCount,
                                                           surfaceFormats.data());

    if(surfaceFormats[0].format != VK_FORMAT_B8G8R8A8_UNORM)
	{
        checkf(0, "Failed creating swap chain, unsupported surface format");
	}

    SurfaceFormatKHR = surfaceFormats[0];
    int width,height = 0;
    SDL_Vulkan_GetDrawableSize(pRenderWindow->GetWindow(), &width, &height);
    width = glm::clamp(width, static_cast<int>(SurfaceCapabilitiesKHR.minImageExtent.width), static_cast<int>(SurfaceCapabilitiesKHR.maxImageExtent.width));
    height = glm::clamp(height, static_cast<int>(SurfaceCapabilitiesKHR.minImageExtent.height), static_cast<int>(SurfaceCapabilitiesKHR.maxImageExtent.height));
    ViewportSize.width = width;
    ViewportSize.height = height;

    uint32_t imageCount = SurfaceCapabilitiesKHR.minImageCount + 1;
    if (SurfaceCapabilitiesKHR.maxImageCount > 0 && imageCount > SurfaceCapabilitiesKHR.maxImageCount)
    {
        imageCount = SurfaceCapabilitiesKHR.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = SurfaceKHR;
    createInfo.minImageCount = SurfaceCapabilitiesKHR.minImageCount;
    createInfo.imageFormat = SurfaceFormatKHR.format;
    createInfo.imageColorSpace = SurfaceFormatKHR.colorSpace;
    createInfo.imageExtent = ViewportSize;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {graphics_QueueFamilyIndex, present_QueueFamilyIndex};
    if (graphics_QueueFamilyIndex != present_QueueFamilyIndex)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = SurfaceCapabilitiesKHR.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;

    vkCreateSwapchainKHR(Device, &createInfo, nullptr, &SwapChain);

    vkGetSwapchainImagesKHR(Device, SwapChain, &SwapChainImageCount, nullptr);
    SwapChainImages.resize(SwapChainImageCount);
    vkGetSwapchainImagesKHR(Device, SwapChain, &SwapChainImageCount, SwapChainImages.data());

    SwapChainImagesViews.resize(SwapChainImages.size());

    for(uint32_t i = 0; i < SwapChainImages.size(); i++)
    {
        SwapChainImagesViews[i] = CreateImageView(SwapChainImages[i], SurfaceFormatKHR.format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

VkBool32 GetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat)
{
    std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto& format : depthFormats)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *depthFormat = format;
            return true;
        }
    }

    return false;
}

void FRenderer::SetupDepthStencil()
{
    VkBool32 validDepthFormat = GetSupportedDepthFormat(PhysicalDevice, &DepthFormat);
    CreateImage(ViewportSize.width, ViewportSize.height, 
                VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                DepthImage, DepthImageMemory);
    DepthImageView = CreateImageView(DepthImage, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void FRenderer::CreateRenderPass()
{
    std::vector<VkAttachmentDescription> attachments(2);

	attachments[0].format = SurfaceFormatKHR.format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].format = DepthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;
	subpassDescription.pResolveAttachments = nullptr;

	vector<VkSubpassDependency> dependencies(1);

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	vkCreateRenderPass(Device, &renderPassInfo, nullptr, &RenderPass);
}

void FRenderer::CreateFrameBuffers()
{
    SwapChainFrameBuffers.resize(SwapChainImagesViews.size());

    for (size_t i = 0; i < SwapChainImagesViews.size(); i++)
    {
        std::vector<VkImageView> attachments(2);
        attachments[0] = SwapChainImagesViews[i];
        attachments[1] = DepthImageView;

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = RenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = ViewportSize.width;
        framebufferInfo.height = ViewportSize.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &SwapChainFrameBuffers[i]) != VK_SUCCESS)
        {
            checkf(0, "CreateFramebuffers: failed to create framebuffer");
        }
    }
}

void FRenderer::CreateCommandPool()
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = graphics_QueueFamilyIndex;
    vkCreateCommandPool(Device, &createInfo, nullptr, &CommandPool);
}

void FRenderer::CreateCommandBuffers()
{
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = CommandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = SwapChainImageCount;

    CommandBuffers.resize(SwapChainImageCount);
    vkAllocateCommandBuffers(Device, &allocateInfo, CommandBuffers.data());
}

void FRenderer::CreateSemaphores()
{
    CreateSemaphore(&ImageAvailableSemaphore);
    CreateSemaphore(&RenderingFinishedSemaphore);
}

void FRenderer::CreateFences()
{
    uint32_t i;
    Fences.resize(SwapChainImageCount);
    for(i = 0; i < SwapChainImageCount; i++)
    {
        VkResult result;

        VkFenceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(Device, &createInfo, nullptr, &Fences[i]);
    }
}

void FRenderer::CreateGBuffer()
{
    GBuffer = FGBuffer();
    VkFormat DepthFormat;
    const bool bSuccess = GetCommandList().GetSupportedDepthFormat(&DepthFormat);
    checkf(bSuccess, "FRenderer::CreateGBuffer getting supported format ");
    
    GBuffer.BufferA = GetCommandList().CreateTexture(ViewportSize.width, ViewportSize.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    GBuffer.BufferB = GetCommandList().CreateTexture(ViewportSize.width, ViewportSize.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    GBuffer.BufferC = GetCommandList().CreateTexture(ViewportSize.width, ViewportSize.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    GBuffer.Depth = GetCommandList().CreateTexture(ViewportSize.width, ViewportSize.height, VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // Set up separate renderpass with references to the color and depth attachments
    std::array<VkAttachmentDescription, 4> attachmentDescs = {};

    // Init attachment properties
    for (uint32_t i = 0; i < 4; ++i)
    {
        attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        if (i == 3)
        {
            attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else
        {
            attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    // Formats
    attachmentDescs[0].format = GBuffer.BufferA.Format;
    attachmentDescs[1].format = GBuffer.BufferB.Format;
    attachmentDescs[2].format = GBuffer.BufferC.Format;
    attachmentDescs[3].format = GBuffer.Depth.Format;

    std::vector<VkAttachmentReference> colorReferences;
    colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 3;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = colorReferences.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pDepthStencilAttachment = &depthReference;

    // Use subpass dependencies for attachment layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments = attachmentDescs.data();
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies.data();

    if(vkCreateRenderPass(Device, &renderPassInfo, nullptr, &GBuffer.RenderPass) != VK_SUCCESS)
    {
        checkf(0, "FRenderer::CreateGBuffer Unable to greate geometry pass");
    }

    std::array<VkImageView,4> attachments;
    attachments[0] = GBuffer.BufferA.ImageView;
    attachments[1] = GBuffer.BufferB.ImageView;
    attachments[2] = GBuffer.BufferC.ImageView;
    attachments[3] = GBuffer.Depth.ImageView;

    VkFramebufferCreateInfo fbufCreateInfo = {};
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.pNext = NULL;
    fbufCreateInfo.renderPass = GBuffer.RenderPass;
    fbufCreateInfo.pAttachments = attachments.data();
    fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbufCreateInfo.width = ViewportSize.width;
    fbufCreateInfo.height = ViewportSize.height;
    fbufCreateInfo.layers = 1;
    
    if(vkCreateFramebuffer(Device, &fbufCreateInfo, nullptr, &GBuffer.FrameBuffer) != VK_SUCCESS)
    {
        checkf(0, "FRenderer::CreateGBuffer Unable to create frame buffer for geomtry pass");
    }

    LOG_Info("Generating GBuffer, success");

    // Setup descriptor layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0 : Vertex shader uniform buffer
        GetCommandList().DescriptorSetLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
        // Binding 1 : Position texture target / Scene colormap
        GetCommandList().DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        // Binding 2 : Normals texture target
        GetCommandList().DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        // Binding 3 : Albedo texture target
        GetCommandList().DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
        // Binding 4 : Fragment shader uniform buffer
        GetCommandList().DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    if(vkCreateDescriptorSetLayout(Device, &descriptorSetLayoutCreateInfo, nullptr, &GBuffer.descriptorSetLayout) != VK_SUCCESS)
    {
        checkf(0, "Unable to create descriptor set layout");
    }

    // Shared pipeline layout used by all pipelines
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &GBuffer.descriptorSetLayout;
    if(vkCreatePipelineLayout(Device, &pipelineLayoutCreateInfo, nullptr, &GBuffer.pipelineLayout) != VK_SUCCESS)
    {
        checkf(0, "Unable to create pipeline layout");
    }

    // Create pipeline
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();

		// Final fullscreen composition pass pipeline
    rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    shaderStages[0] = loadShader(getShadersPath() + "deferred/deferred.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShadersPath() + "deferred/deferred.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    // Empty vertex input state, vertices are generated by the vertex shader
    VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
    pipelineCI.pVertexInputState = &emptyInputState;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.composition));

    // Vertex input state from glTF model for pipeline rendering models
    pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Tangent});
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

		// Offscreen pipeline
    shaderStages[0] = loadShader(getShadersPath() + "deferred/mrt.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShadersPath() + "deferred/mrt.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		// Separate render pass
    pipelineCI.renderPass = offScreenFrameBuf.renderPass;

		// Blend attachment states required for all color attachments
		// This is important, as color write mask will otherwise be 0x0 and you
		// won't see anything rendered to the attachment
    std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
		};

		colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		colorBlendState.pAttachments = blendAttachmentStates.data();

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.offscreen));
}

void FRenderer::CreateSemaphore(VkSemaphore* Semaphore)
{
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(Device, &createInfo, nullptr, Semaphore);
}

void FRenderer::RenderGeometryPass()
{
    if (GBuffer.GeometryPassCommand == VK_NULL_HANDLE)
    {
        GBuffer.GeometryPassCommand = GetCommandList().CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
    }

		// Create a semaphore used to synchronize offscreen rendering and usage
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if(vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &GBuffer.GeometryPassSemaphore) != VK_SUCCESS)
    {
        checkf(0, "Unable to create semaphore for geometry pass");
    }

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // Clear values for all attachments written in the fragment shader
    std::array<VkClearValue,4> clearValues;
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[3].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass =  GBuffer.RenderPass;
    renderPassBeginInfo.framebuffer = GBuffer.FrameBuffer;
    renderPassBeginInfo.renderArea.extent.width = ViewportSize.width;
    renderPassBeginInfo.renderArea.extent.height = ViewportSize.height;
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    if(vkBeginCommandBuffer(GBuffer.GeometryPassCommand, &cmdBufInfo) != VK_SUCCESS)
    {
        checkf(0, "Unable to begin command buffer for geometry pass");
    }

    vkCmdBeginRenderPass(GBuffer.GeometryPassCommand, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport Viewport {};
    Viewport.width = ViewportSize.width;
    Viewport.height = ViewportSize.height;
    Viewport.minDepth = 0.0f;
    Viewport.maxDepth = 1.0f;
   
    vkCmdSetViewport(GBuffer.GeometryPassCommand, 0, 1, &Viewport);

    VkRect2D scissor {};
    scissor.extent.width = ViewportSize.width;
    scissor.extent.height = ViewportSize.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    
    vkCmdSetScissor(GBuffer.GeometryPassCommand, 0, 1, &scissor);

    vkCmdBindPipeline(GBuffer.GeometryPassCommand, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);

    // Render fucking object here
    vkCmdBindDescriptorSets(GBuffer.GeometryPassCommand, VK_PIPELINE_BIND_POINT_GRAPHICS,  GBuffer.pipelineLayout, 0, 1, &descriptorSets.model, 0, nullptr);
    models.model.bindBuffers(offScreenCmdBuffer);
    vkCmdDrawIndexed(GBuffer.GeometryPassCommand, models.model.indices.count, 1, 0, 0, 0);

    vkCmdEndRenderPass(GBuffer.GeometryPassCommand);

    if(vkEndCommandBuffer(GBuffer.GeometryPassCommand) != VK_SUCCESS)
    {
        checkf(0, "Unable to finish command buffer for geometry pass");
    }
}

