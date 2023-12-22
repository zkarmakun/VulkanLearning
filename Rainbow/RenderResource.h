#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_core.h>

struct FStaticVertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 UV0;
    glm::vec3 Color;
    
    FStaticVertex()
    {
        Position = glm::vec3(0);
        Normal = glm::vec3(0);
        UV0 = glm::vec2(0);
        Color = glm::vec3(0);
    }
};


struct FVertexBuffer
{
public:
    VkBuffer VertexBuffer;
    VkDeviceMemory VertexMemory;
    int VertexBufferSize;

    VkBuffer IndexBuffer;
    VkDeviceMemory IndexMemory;
    int IndexBufferSize;

    FVertexBuffer()
    {
        VertexBuffer = nullptr;
        VertexMemory = nullptr;
        VertexBufferSize = 0;

        IndexBuffer = nullptr;
        IndexMemory = nullptr;
        IndexBufferSize = 0;
    }
};

class FTexture
{
public:
    VkImage Image;
    VkDeviceMemory ImageMemory;
    VkImageView ImageView;
    VkFormat Format;
    uint32_t SizeX, SizeY;
    uint32_t MipMaps;
    VkSampler Sampler;
    VkImageLayout ImageLayout;
    
    FTexture()
    {
        Image = nullptr;
        ImageMemory = nullptr;
        ImageView = nullptr;
        Format = VK_FORMAT_UNDEFINED;
        SizeX = 0;
        SizeY = 0;
        MipMaps = 0;
        Sampler = nullptr;
        ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
};

