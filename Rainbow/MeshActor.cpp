#include "MeshActor.h"
#include <vector>
#include "FbxImport.h"
#include "CommandList.h"
#include "RenderResource.h"
#include "Renderer.h"

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

        return;
    }
}

bool FMeshActor::IsValid() const
{
    return VertexBuffer && VertexBuffer->VertexBuffer != VK_NULL_HANDLE;
}
