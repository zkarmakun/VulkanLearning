#pragma once
#include "Core/MinimalCore.h"
#include <memory>
#include <vector>
#include <glm/vec3.hpp>

class FActor;

class FWorld
{
public:
    void LoadWorld();
    void Render();

    template<class ActorClass>
    std::shared_ptr<FActor> CreateActor(glm::vec3 Location, glm::vec3 Rotation, glm::vec3 Scale = glm::vec3(1));
    std::vector<std::shared_ptr<FActor>> GetActors();

private:
    std::vector<std::shared_ptr<FActor>> Actors;
};

