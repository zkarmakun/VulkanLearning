#pragma once
#include "Actor.h"
#include "RenderResource.h"

class FMeshActor : public FActor
{
public:
    FMeshActor();
    
    virtual void LoadActor(std::string FilePath) override;
    virtual bool IsValid() const override;

    
private:
    FVertexBuffer* VertexBuffer;
};
