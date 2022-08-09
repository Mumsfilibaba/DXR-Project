#include "OBJLoader.h"
#include "StbImageLoader.h"

#include "Core/Math/MathCommon.h"
#include "Core/Containers/HashTable.h"
#include "Core/Utilities/StringUtilities.h"
#include "Core/Threading/AsyncTaskManager.h"
#include "Core/Misc/OutputDeviceLogger.h"

#include "Engine/Assets/MeshUtilities.h"

#include <tiny_obj_loader.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FOBJLoader

struct FOBJTextureContext
{
    int8 LoadMaterialTexture(const FString& Path, const FString& Filename)
    {
        // If filename is empty there is no texture to load
        if (Filename.IsEmpty())
        {
            return -1;
        }

        // Make sure that correct slashes are used
        FString Fullpath = Path + '/' + Filename;
        ConvertBackslashes(Fullpath);

        auto TextureIt = UniqueTextures.find(Fullpath);
        if (TextureIt == UniqueTextures.end())
        {
            int8 TextureIndex = static_cast<int8>(Textures.GetSize());

            FImage2DPtr NewTexture = Textures.Emplace(FSTBImageLoader::LoadFile(Fullpath));
            UniqueTextures.insert(std::make_pair(Fullpath, TextureIndex));
            return TextureIndex;
        }
        else
        {
            return TextureIt->second;
        }
    }

    THashTable<FString, int8, FStringHasher> UniqueTextures;
    TArray<FImage2DPtr>                      Textures;
};

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

    FString MTLFiledir = FString(Filename.CStr(), Filename.ReverseFind('/'));
    if (!tinyobj::LoadObj(&Attributes, &Shapes, &Materials, &Warning, &Error, Filename.CStr(), MTLFiledir.CStr(), true, false))
    {
        LOG_WARNING("[FOBJLoader]: Failed to load '%s'. Warning: %s Error: %s", Filename.CStr(), Warning.c_str(), Error.c_str());
        return false;
    }
    else
    {
        LOG_INFO("[FOBJLoader]: Loaded '%s'", Filename.CStr());
    }

    // Create All Materials in scene
    FOBJTextureContext TextureContext;
    for (tinyobj::material_t& Mat : Materials)
    {
        // Create new material with default properties
        FMaterialData MaterialData;
        MaterialData.MetallicTexture  = TextureContext.LoadMaterialTexture(MTLFiledir, Mat.ambient_texname.c_str());
        MaterialData.DiffuseTexture   = TextureContext.LoadMaterialTexture(MTLFiledir, Mat.diffuse_texname.c_str());
        MaterialData.RoughnessTexture = TextureContext.LoadMaterialTexture(MTLFiledir, Mat.specular_highlight_texname.c_str());
        MaterialData.NormalTexture    = TextureContext.LoadMaterialTexture(MTLFiledir, Mat.bump_texname.c_str());
        MaterialData.AlphaMaskTexture = TextureContext.LoadMaterialTexture(MTLFiledir, Mat.alpha_texname.c_str());

        MaterialData.Diffuse   = FVector3(Mat.diffuse[0], Mat.diffuse[1], Mat.diffuse[2]);
        MaterialData.Metallic  = Mat.ambient[0];
        MaterialData.AO        = 1.0f;
        MaterialData.Roughness = 1.0f;

        OutScene.Materials.Emplace(MaterialData);
    }
    OutScene.Textures = TextureContext.Textures;

    // Construct Scene
    FModelData Data;
    THashTable<FVertex, uint32, FVertexHasher> UniqueVertices;
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

    OutScene.Models.ShrinkToFit();
    OutScene.Materials.ShrinkToFit();

    FAsyncTaskManager::Get().WaitForAll();
    return true;
}
