#include "FbxImport.h"
#include <fbxsdk.h>
#include <glm/vec4.hpp>


bool FFbxImport::GetMeshData(
    const std::string FilePath,
    std::vector<glm::vec3>& VertexPosition,
    std::vector<uint32_t>& Indices,
    std::vector<glm::vec3>& VertexNormals,
    std::vector<glm::vec2>& UV0,
    std::vector<glm::vec3>& VertexColors)
{
    FbxManager* pManager = FbxManager::Create();
    //Create an IOSettings object. This object holds all import/export settings.
    FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
    pManager->SetIOSettings(ios);

    // Load the FBX file
    FbxImporter* importer = FbxImporter::Create(pManager, "");
    if (!importer->Initialize(FilePath.c_str()))
    {
        return false;
    }

    FbxScene* scene = FbxScene::Create(pManager, "myScene");
    importer->Import(scene);
    importer->Destroy();

    FbxNode* rootNode = scene->GetRootNode();
    if (rootNode) {
        // Iterate through the scene nodes to find meshes
        for (int i = 0; i < rootNode->GetChildCount(); i++) {
            FbxNode* node = rootNode->GetChild(i);
            FbxMesh* mesh = node->GetMesh();
            if (mesh) {
                // Extract vertex positions
                FbxVector4* vertices = mesh->GetControlPoints();
                int vertexCount = mesh->GetControlPointsCount();
                for (int j = 0; j < vertexCount; j++) {
                    FbxVector4 vertex = vertices[j];
                    glm::vec3 position(vertex[0], vertex[1], vertex[2]);
                    VertexPosition.push_back(position);
                }

                FbxGeometryElementNormal* normals = mesh->GetElementNormal();
                if (normals) {
                    for (int j = 0; j < vertexCount; j++) {
                        int normalIndex = (normals->GetReferenceMode() == FbxGeometryElement::eDirect) ? j : normals->GetIndexArray().GetAt(j);
                        FbxVector4 normal = normals->GetDirectArray().GetAt(normalIndex);
                        glm::vec3 normalVec(normal[0], normal[1], normal[2]);
                        VertexNormals.push_back(normalVec);
                    }
                }

                // Extract UV coordinates (index 0)
                FbxGeometryElementUV* uvElement = mesh->GetElementUV(0);
                if (uvElement) {
                    for (int j = 0; j < vertexCount; j++) {
                        int uvIndex = (uvElement->GetReferenceMode() == FbxGeometryElement::eDirect) ? j : uvElement->GetIndexArray().GetAt(j);
                        FbxVector2 uv = uvElement->GetDirectArray().GetAt(uvIndex);
                        glm::vec2 uvCoord(uv[0], uv[1]);
                        UV0.push_back(uvCoord);
                    }
                }

                // Extract vertex colors
                FbxGeometryElementVertexColor* vertexColorElement = mesh->GetElementVertexColor();
                if (vertexColorElement) {
                    for (int j = 0; j < vertexCount; j++) {
                        int colorIndex = (vertexColorElement->GetReferenceMode() == FbxGeometryElement::eDirect) ? j : vertexColorElement->GetIndexArray().GetAt(j);
                        FbxColor color = vertexColorElement->GetDirectArray().GetAt(colorIndex);
                        glm::vec4 colorVec(color.mRed, color.mGreen, color.mBlue, color.mAlpha);
                        VertexColors.push_back(colorVec);
                    }
                }

                // Extract indices
                int polygonCount = mesh->GetPolygonCount();
                for (int j = 0; j < polygonCount; j++) {
                    int polygonSize = mesh->GetPolygonSize(j);
                    for (int k = 0; k < polygonSize; k++) {
                        int index = mesh->GetPolygonVertex(j, k);
                        Indices.push_back(index);
                    }
                }
            }
        }
    }

    // Clean up resources
    scene->Destroy();
    ios->Destroy();
    pManager->Destroy();
    return true;
}
