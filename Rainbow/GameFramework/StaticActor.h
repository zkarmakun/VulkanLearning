#pragma once
#include "Actor.h"
#include "Renderer/RenderResource.h"

class FStaticActor : public FActor
{
public:
    FStaticActor();
    
    virtual void LoadActor(std::string FilePath) override;
    virtual bool IsValid() const override;
    
private:
    FVertexBuffer* VertexBuffer;
};
