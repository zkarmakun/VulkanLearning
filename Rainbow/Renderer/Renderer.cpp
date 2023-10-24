#pragma once
#include "Renderer.h"
#include <glm/glm.hpp>
#include <set>
#include <SDL2/SDL_vulkan.h>
#include <vector>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_timer.h>
#include <vulkan/vulkan_core.h>

#include "CommandList.h"
#include "RenderWindow.h"

FCommandList FRenderer::CmdList;

FRenderer::FRenderer()
{
    bInitialized = false;
    pRenderWindow = nullptr;
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

    LOG_Info("Initializing vulkan completed");

    CmdList = FCommandList(this);
    bInitialized = true;
}

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
            VkClearColorValue ClearColor = {0.2f, 0.2f, 0.2f, 1.0f};
            VkClearDepthStencilValue ClearDepthStencilValue = {1.0f, 0};
            GetCommandList().BeginRenderPass(ClearColor, ClearDepthStencilValue);
            {
                
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
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, MemoryPropertyFlags);

    if (vkAllocateMemory(Device, &allocInfo, nullptr, &ImageMemory) != VK_SUCCESS)
    {
        check(0);
    }

    vkBindImageMemory(Device, Image, ImageMemory, 0);
}

uint32_t FRenderer::FindMemoryType(uint32_t TypeFilter, VkMemoryPropertyFlags MemoryPropertyFlags)
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

VkDevice& FRenderer::GetDevice()
{
    return Device;
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

VkExtent2D& FRenderer::GetSwapChainSize()
{
    return SwapChainSize;
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
    appInfo.pEngineName = "No Engine";
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
    SwapChainSize.width = width;
    SwapChainSize.height = height;

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
    createInfo.imageExtent = SwapChainSize;
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
    CreateImage(SwapChainSize.width, SwapChainSize.height, 
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
        framebufferInfo.width = SwapChainSize.width;
        framebufferInfo.height = SwapChainSize.height;
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

void FRenderer::CreateSemaphore(VkSemaphore* Semaphore)
{
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(Device, &createInfo, nullptr, Semaphore);
}

