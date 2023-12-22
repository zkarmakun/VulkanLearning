#pragma once
#include "MinimalCore.h"
#include "RenderResource.h"
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
