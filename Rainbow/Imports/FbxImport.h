#pragma once
#include <string>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class FFbxImport
{
public:
    static bool GetMeshData(
        const std::string FilePath,
        std::vector<glm::vec3>& VertexPosition,
        std::vector<uint32_t>& Indices,
        std::vector<glm::vec3>& VertexNormals,
        std::vector<glm::vec2>& UV0,
        std::vector<glm::vec3>& VertexColors);
};
