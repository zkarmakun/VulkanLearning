#pragma once
#include <string>
#include <glm/vec3.hpp>

class FWorld;

class FActor
{
public:
    FActor();
    
    void SetLocation(glm::vec3 NewLocation);
    void SetRotation(glm::vec3 NewRotation);
    void SetScale(glm::vec3 NewScale);

    glm::vec3 GetLocation() const { return Location; }
    glm::vec3 GetRotation() const { return Rotation; }
    glm::vec3 GetScale() const { return Scale;}

    FWorld* GetWorld() const;
    
    virtual bool IsValid() const;
    virtual void LoadActor(std::string FilePath);

private:
    void SetWorld(FWorld* InWorld);
    
private:
    bool bValid;
    glm::vec3 Location;
    glm::vec3 Rotation;
    glm::vec3 Scale;

    FWorld* World;
    friend class FWorld;
};
