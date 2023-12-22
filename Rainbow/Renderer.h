#pragma once
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vector>
#include "RenderResource.h"
#include "MinimalCore.h"

struct FGBuffer
{
    int32_t Width, Height;
    VkFramebuffer FrameBuffer;
    FTexture BufferA;
    FTexture BufferB;
    FTexture BufferC;
    FTexture BufferD;
    FTexture Depth;
    VkRenderPass RenderPass;
    VkSampler Sampler;
    VkCommandBuffer GeometryPassCommand;
    VkSemaphore GeometryPassSemaphore;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;

    FGBuffer()
    {
        Width = 0;
        Height = 0;
        FrameBuffer = nullptr;
        RenderPass = nullptr;
        Sampler = nullptr;
        GeometryPassCommand = VK_NULL_HANDLE;
        GeometryPassSemaphore = VK_NULL_HANDLE;
    }
};

class FWorld;
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
    static uint32_t FindMemoryType(const VkPhysicalDevice& PhysicalDevice, uint32_t TypeFilter, VkMemoryPropertyFlags MemoryPropertyFlags);
    uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const;

public:
    VkDevice& GetDevice();
    VkPhysicalDevice& GetPhysicalDevice();
    VkSwapchainKHR& GetSwapChain();
    VkSemaphore& GetImageAvailableSemaphore();
    VkSemaphore& GetRenderingFinishedSemaphore();
    std::vector<VkFence>& GetFences();
    std::vector<VkCommandBuffer>& GetCommandBuffers();
    std::vector<VkImage>& GetSwapChainImages();
    VkCommandPool& GetCommandPool();
    VkRenderPass& GetRenderPass();
    std::vector<VkFramebuffer>& GetSwapChainFrameBuffers();
    VkExtent2D& GetViewportSize();
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
    void CreateGBuffer();

    void CreateSemaphore(VkSemaphore *Semaphore);

    // Render
    void RenderGeometryPass();

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

    VkSwapchainKHR SwapChain;
    VkExtent2D ViewportSize;
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


    FWorld* World;
    
    static FCommandList CmdList;

    uint32_t FrameIndex;
    VkCommandBuffer CommandBuffer;

    FGBuffer GBuffer;
};
