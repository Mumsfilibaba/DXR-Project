#include "OBJLoader.h"

#include "Core/Math/MathCommon.h"
#include "Core/Containers/Map.h"
#include "Core/Utilities/StringUtilities.h"
#include "Core/Threading/AsyncTask.h"
#include "Core/Misc/OutputDeviceLogger.h"

#include "Engine/Assets/MeshUtilities.h"
#include "Engine/Assets/AssetManager.h"

#include <tiny_obj_loader.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FOBJLoader

bool FOBJLoader::LoadFile(const FString& Filename, FSceneData& OutScene, bool ReverseHandedness)
{
    // Make sure to clear everything
    OutScene.Models.Clear();
    OutScene.Materials.Clear();

    // Load Scene File
    std::string                      Warning;
    std::string                      Error;
    std::vector<tinyobj::shape_t>    Shapes;
    std::vector<tinyobj::material_t> Materials;
    tinyobj::attrib_t                Attributes;

    FString MTLFiledir = FString(Filename.GetCString(), Filename.FindLastChar('/'));
    if (!tinyobj::LoadObj(&Attributes, &Shapes, &Materials, &Warning, &Error, Filename.GetCString(), MTLFiledir.GetCString(), true, false))
    {
        LOG_WARNING("[FOBJLoader]: Failed to load '%s'. Warning: %s Error: %s", Filename.GetCString(), Warning.c_str(), Error.c_str());
        return false;
    }
    else
    {
        LOG_INFO("[FOBJLoader]: Loaded '%s'", Filename.GetCString());
    }

    // Create All Materials in scene
    for (tinyobj::material_t& Mat : Materials)
    {
        // Create new material with default properties
        FMaterialData MaterialData;
        MaterialData.MetallicTexture = StaticCastSharedRef<FTextureResource2D>(
            FAssetManager::Get().LoadTexture(MTLFiledir + '/' + Mat.ambient_texname.c_str()));
        MaterialData.DiffuseTexture = StaticCastSharedRef<FTextureResource2D>(
            FAssetManager::Get().LoadTexture(MTLFiledir + '/' + Mat.diffuse_texname.c_str()));
        MaterialData.RoughnessTexture = StaticCastSharedRef<FTextureResource2D>(
            FAssetManager::Get().LoadTexture(MTLFiledir + '/' + Mat.specular_highlight_texname.c_str()));
        MaterialData.NormalTexture = StaticCastSharedRef<FTextureResource2D>(
            FAssetManager::Get().LoadTexture(MTLFiledir + '/' + Mat.bump_texname.c_str()));
        MaterialData.AlphaMaskTexture = StaticCastSharedRef<FTextureResource2D>(
            FAssetManager::Get().LoadTexture(MTLFiledir + '/' + Mat.alpha_texname.c_str()));


        MaterialData.Diffuse   = FVector3(Mat.diffuse[0], Mat.diffuse[1], Mat.diffuse[2]);
        MaterialData.Metallic  = Mat.ambient[0];
        MaterialData.AO        = 1.0f;
        MaterialData.Roughness = 1.0f;

        OutScene.Materials.Emplace(MaterialData);
    }

    // Construct Scene
    FModelData Data;
    TMap<FVertex, uint32, FVertexHasher> UniqueVertices;
    for (const tinyobj::shape_t& Shape : Shapes)
    {
        // Start at index zero for each mesh and loop until all indices are processed
        uint32 i = 0;
        uint32 IndexCount = static_cast<uint32>(Shape.mesh.indices.size());
        while (i < IndexCount)
        {
            // Start a new mesh
            Data.Mesh.Clear();
            UniqueVertices.clear();

            Data.Mesh.Indices.Reserve(IndexCount);

            int32 Face = i / 3;
            const int32 MaterialID = Shape.mesh.material_ids[Face];
            for (; i < IndexCount; i++)
            {
                // Break if material is not the same
                Face = i / 3;
                if (Shape.mesh.material_ids[Face] != MaterialID)
                {
                    break;
                }

                const tinyobj::index_t& Index = Shape.mesh.indices[i];

                FVertex TempVertex;

                // Normals and texcoords are optional, Positions are required
                Check(Index.vertex_index >= 0);

                auto PositionIndex = 3 * Index.vertex_index;
                TempVertex.Position = FVector3(Attributes.vertices[PositionIndex + 0], Attributes.vertices[PositionIndex + 1], Attributes.vertices[PositionIndex + 2]);

                if (Index.normal_index >= 0)
                {
                    auto NormalIndex = 3 * Index.normal_index;
                    TempVertex.Normal = FVector3(Attributes.normals[NormalIndex + 0], Attributes.normals[NormalIndex + 1], Attributes.normals[NormalIndex + 2]);
                    TempVertex.Normal.Normalize();
                }

                if (Index.texcoord_index >= 0)
                {
                    auto TexCoordIndex = 2 * Index.texcoord_index;
                    TempVertex.TexCoord = FVector2(Attributes.texcoords[TexCoordIndex + 0], Attributes.texcoords[TexCoordIndex + 1]);
                }

                if (UniqueVertices.count(TempVertex) == 0)
                {
                    UniqueVertices[TempVertex] = static_cast<uint32>(Data.Mesh.Vertices.GetSize());
                    Data.Mesh.Vertices.Push(TempVertex);
                }

                Data.Mesh.Indices.Emplace(UniqueVertices[TempVertex]);
            }

            // Calculate tangents and create mesh
            FMeshUtilities::CalculateTangents(Data.Mesh);

            if (ReverseHandedness)
            {
                FMeshUtilities::ReverseHandedness(Data.Mesh);
            }

            // Add materialID
            if (MaterialID >= 0)
            {
                Data.MaterialIndex = MaterialID;
            }

            Data.Name = Shape.name.c_str();
            OutScene.Models.Emplace(Data);
        }
    }

    OutScene.Models.Shrink();
    OutScene.Materials.Shrink();
    return true;
}
