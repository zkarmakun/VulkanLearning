#include "MeshActor.h"
#include <vector>
#include "Imports/FbxImport.h"
#include "Renderer/CommandList.h"
#include "Renderer/RenderResource.h"
#include "Renderer/Renderer.h"

FMeshActor::FMeshActor()
{
    VertexBuffer = nullptr;
}

void FMeshActor::LoadActor(std::string FilePath)
{
    FActor::LoadActor(FilePath);
    std::vector<uint32_t> IndicesData;
    std::vector<FStaticVertex> VertexData;
    if(FFbxImport::GetStaticMeshData(FilePath, VertexData, IndicesData))
    {
        LOG_Info("Loading static mesh, VertexData:%i, IndicesData:%i", VertexData.size(), IndicesData.size());
        VertexBuffer = FRenderer::GetCommandList().CreateVertexBuffer(VertexData, IndicesData);
    }
}

bool FMeshActor::IsValid() const
{
    return VertexBuffer && VertexBuffer->VertexBuffer != VK_NULL_HANDLE;
}
