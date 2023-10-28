#include "World.h"

#include "Actor.h"
#include "MeshActor.h"
#include "Core/Paths.h"

void FWorld::LoadWorld()
{
    const std::shared_ptr<FActor> NewMesh = CreateActor<FMeshActor>(glm::vec3(0), glm::vec3(0));
    NewMesh->SetWorld(this);
    NewMesh->LoadActor(FPaths::GetContentDirectory() + "/Suzan.fbx");
    Actors.push_back(NewMesh);
}

void FWorld::Render()
{
    for(auto& Actor : Actors)
    {
        if(Actor->IsValid())
        {
            // LOG_Info("Render the shit out of this");
        }
    }
}

std::vector<std::shared_ptr<FActor>> FWorld::GetActors()
{
    return Actors;
}

template <class ActorClass>
std::shared_ptr<FActor> FWorld::CreateActor(glm::vec3 Location, glm::vec3 Rotation, glm::vec3 Scale)
{
    std::shared_ptr<ActorClass> NewActor = std::make_shared<ActorClass>();
    NewActor->SetLocation(Location);
    NewActor->SetRotation(Rotation);
    NewActor->SetScale(Scale);
    return NewActor;
}


