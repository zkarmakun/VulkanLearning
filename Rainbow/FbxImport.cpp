#include "FbxImport.h"
#include <fbxsdk.h>
#include <glm/vec4.hpp>

#include "RenderResource.h"

bool FFbxImport::GetStaticMeshData(
    const std::string FilePath,
    std::vector<FStaticVertex>& Vertices,
    std::vector<uint32_t>& Indices)
{
    Vertices.clear();
    Indices.clear();
    
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
                FbxGeometryElementNormal* normals = mesh->GetElementNormal();
                FbxGeometryElementUV* uvElement = mesh->GetElementUV(0);
                FbxGeometryElementVertexColor* vertexColorElement = mesh->GetElementVertexColor();
                
                int vertexCount = mesh->GetControlPointsCount();
                for (int j = 0; j < vertexCount; j++)
                {
                    FStaticVertex StaticVertex;
                    // Get Position
                    FbxVector4 vertex = vertices[j];
                    glm::vec3 position(vertex[0], vertex[1], vertex[2]);
                    StaticVertex.Position = position;

                    // Get Normals
                    int normalIndex = (normals->GetReferenceMode() == FbxGeometryElement::eDirect) ? j : normals->GetIndexArray().GetAt(j);
                    FbxVector4 normal = normals->GetDirectArray().GetAt(normalIndex);
                    glm::vec3 normalVec(normal[0], normal[1], normal[2]);
                    StaticVertex.Normal = normalVec;

                    // Get UV0
                    int uvIndex = (uvElement->GetReferenceMode() == FbxGeometryElement::eDirect) ? j : uvElement->GetIndexArray().GetAt(j);
                    FbxVector2 uv = uvElement->GetDirectArray().GetAt(uvIndex);
                    glm::vec2 uvCoord(uv[0], uv[1]);
                    StaticVertex.UV0 = uvCoord;

                    if(vertexColorElement != NULL)
                    {
                        int colorIndex = (vertexColorElement->GetReferenceMode() == FbxGeometryElement::eDirect) ? j : vertexColorElement->GetIndexArray().GetAt(j);
                        FbxColor color = vertexColorElement->GetDirectArray().GetAt(colorIndex);
                        glm::vec4 colorVec(color.mRed, color.mGreen, color.mBlue, color.mAlpha);
                        StaticVertex.Color = colorVec;
                    }

                    Vertices.push_back(StaticVertex);
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
