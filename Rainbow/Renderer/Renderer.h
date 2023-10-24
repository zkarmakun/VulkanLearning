#pragma once
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vector>
#include "Core/MinimalCore.h"

class FRenderWindow;
class FCommandList;

class FRenderer
{
public:
    FRenderer();
    void Init(FRenderWindow* RenderWindow);
    void RenderLoop();
    void Shutdown();

    VkImageView CreateImageView(VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags);
    void CreateImage(uint32_t Width, uint32_t Height, VkFormat Format, VkImageTiling Tiling, VkImageUsageFlags ImageUsageFlags, VkMemoryPropertyFlags MemoryPropertyFlags, VkImage& Image, VkDeviceMemory& ImageMemory);
    uint32_t FindMemoryType(uint32_t TypeFilter, VkMemoryPropertyFlags MemoryPropertyFlags);

public:
    VkDevice& GetDevice();
    VkSwapchainKHR& GetSwapChain();
    VkSemaphore& GetImageAvailableSemaphore();
    VkSemaphore& GetRenderingFinishedSemaphore();
    std::vector<VkFence>& GetFences();
    std::vector<VkCommandBuffer>& GetCommandBuffers();
    std::vector<VkImage>& GetSwapChainImages();
    VkCommandPool& GetCommandPool();
    VkRenderPass& GetRenderPass();
    std::vector<VkFramebuffer>& GetSwapChainFrameBuffers();
    VkExtent2D& GetSwapChainSize();
    VkQueue& GetGraphicsQueue();
    VkQueue& GetPresentQueue();
    static FCommandList& GetCommandList();

private:
    void CreateInstance();
    void CreateDebug();
    void CreateSurface();
    void SelectPhysicalDevice();
    void SelectQueueFamily();
    void CreateDevice();
    void CreateSwapChain();
    void SetupDepthStencil();
    void CreateRenderPass();
    void CreateFrameBuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSemaphores();
    void CreateFences();

    void CreateSemaphore(VkSemaphore *Semaphore);

private:
    bool bInitialized;
    FRenderWindow* pRenderWindow;

    VkInstance Instance;
    VkDebugReportCallbackEXT debugCallback;
    VkSurfaceKHR SurfaceKHR;
    VkSurfaceFormatKHR SurfaceFormatKHR;
    VkSurfaceCapabilitiesKHR SurfaceCapabilitiesKHR;
    VkPhysicalDevice PhysicalDevice;
    uint32_t graphics_QueueFamilyIndex;
    uint32_t present_QueueFamilyIndex;
    
    VkDevice Device;
    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    
    VkPhysicalDeviceProperties PhysicalDeviceProperties;
    VkPhysicalDeviceFeatures PhysicalDeviceFeatures;
    VkPhysicalDeviceMemoryProperties PhysicalDeviceMemoryProperties;

    VkSwapchainKHR SwapChain;
    VkExtent2D SwapChainSize;
    uint32_t SwapChainImageCount;
    std::vector<VkImage> SwapChainImages;
    std::vector<VkImageView> SwapChainImagesViews;
    VkFormat DepthFormat;
    VkImage DepthImage;
    VkDeviceMemory DepthImageMemory;
    VkImageView DepthImageView;

    VkRenderPass RenderPass;
    std::vector<VkFramebuffer> SwapChainFrameBuffers;
    
    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;

    VkSemaphore ImageAvailableSemaphore;
    VkSemaphore RenderingFinishedSemaphore;

    std::vector<VkFence> Fences;
    std::vector<const char*> validationLayers;

    static FCommandList CmdList;
};
