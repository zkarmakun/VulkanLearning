#include "Actor.h"

FActor::FActor()
{
    bValid = false;
    World = nullptr;
}

void FActor::SetLocation(glm::vec3 NewLocation)
{
    Location = NewLocation;
}

void FActor::SetRotation(glm::vec3 NewRotation)
{
    Rotation = NewRotation;
}

void FActor::SetScale(glm::vec3 NewScale)
{
    Scale = NewScale;
}

FWorld* FActor::GetWorld() const
{
    return World;
}

bool FActor::IsValid() const
{
    return bValid;
}

void FActor::LoadActor(std::string FilePath)
{
}

void FActor::SetWorld(FWorld* InWorld)
{
    World = InWorld;
}
