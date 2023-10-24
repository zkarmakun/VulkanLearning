#pragma once
#include "Core/MinimalCore.h"
#include "Renderer/RenderResource.h"
#include <string>
#include <vector>

class FFbxImport
{
public:
    static bool GetStaticMeshData(
        const std::string FilePath,
        std::vector<FStaticVertex>& Vertices,
        std::vector<uint32_t>& Indices);
};
