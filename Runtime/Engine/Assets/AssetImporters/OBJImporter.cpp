#include "Core/Math/MathCommon.h"
#include "Core/Containers/Map.h"
#include "Core/Utilities/StringUtilities.h"
#include "Core/Threading/AsyncTask.h"
#include "Core/Generic/GenericPlatformFile.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Engine/Assets/MeshUtilities.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetImporters/OBJImporter.h"

#include <tiny_obj_loader.h>

TSharedRef<FModel> FOBJImporter::ImportFromFile(const FStringView& InFileName, EMeshImportFlags Flags)
{
    // Load Scene File
    std::string                      Warning;
    std::string                      Error;
    std::vector<tinyobj::shape_t>    Shapes;
    std::vector<tinyobj::material_t> Materials;
    tinyobj::attrib_t                Attributes;

    // Extract just the name of the file
    const FString Filename            = FString(InFileName);
    const FString MTLFiledir          = FFileHelpers::ExtractFilepath(Filename);
    const FString FilenameWithoutPath = FFileHelpers::ExtractFilenameWithoutExtension(Filename);
    
    // Load the OBJ file
    if (!tinyobj::LoadObj(&Attributes, &Shapes, &Materials, &Warning, &Error, Filename.GetCString(), MTLFiledir.GetCString(), true, false))
    {
        LOG_ERROR("[FOBJImporter]: Failed to load '%s'. Warning: %s Error: %s", Filename.GetCString(), Warning.c_str(), Error.c_str());
        return nullptr;
    }
    
    if (!Warning.empty())
    {
        LOG_WARNING("[FOBJImporter]: Loaded '%s' with Warning: %s", Filename.GetCString(), Warning.c_str());
    }
    
    // Create new scene
    TSharedPtr<FImportedModel> Scene = MakeSharedPtr<FImportedModel>();
    
    // Create All Materials in scene
    int32 SceneMaterialIndex = 0;
    for (tinyobj::material_t& Mat : Materials)
    {
        // Create new material with default properties
        FImportedMaterial MaterialData;
        MaterialData.MetallicTexture  = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(MTLFiledir + '/' + Mat.ambient_texname.c_str()));
        MaterialData.DiffuseTexture   = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(MTLFiledir + '/' + Mat.diffuse_texname.c_str()));
        MaterialData.RoughnessTexture = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(MTLFiledir + '/' + Mat.specular_highlight_texname.c_str()));
        MaterialData.NormalTexture    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(MTLFiledir + '/' + Mat.bump_texname.c_str()));
        MaterialData.AlphaMaskTexture = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(MTLFiledir + '/' + Mat.alpha_texname.c_str()));

        MaterialData.Diffuse       = FVector3(Mat.diffuse[0], Mat.diffuse[1], Mat.diffuse[2]);
        MaterialData.Metallic      = Mat.ambient[0];
        MaterialData.AO            = 1.0f;
        MaterialData.Roughness     = 1.0f;
        MaterialData.MaterialFlags = EMaterialFlags::None;

        if (Mat.name.empty())
        {
            MaterialData.Name = FString::CreateFormatted("%s_material_%d", FilenameWithoutPath.GetCString(), SceneMaterialIndex);
        }
        else
        {
            MaterialData.Name = Mat.name.c_str();
        }
        
        Scene->Materials.Emplace(Move(MaterialData));
        SceneMaterialIndex++;
    }

    // Construct Scene
    FMeshData Data;
    TMap<FVertex, uint32> UniqueVertices;
    
    constexpr uint32 NumIndiciesPerTriangle  = 3;
    constexpr uint32 NumPositionsPerTriangle = 3;
    constexpr uint32 NumNormalsPerTriangle   = 3;
    constexpr uint32 NumTexCoordsPerTriangle = 2;

    int32 ShapeIndex = 0;
    for (const tinyobj::shape_t& Shape : Shapes)
    {
        // Start at index zero for each mesh and loop until all indices are processed
        const uint32 IndexCount = static_cast<uint32>(Shape.mesh.indices.size());

        // Start a new mesh
        Data.Clear();
        Data.Indices.Reserve(IndexCount);
        UniqueVertices.Clear();

        uint32 CurrentIndex = 0;
        while (CurrentIndex < IndexCount)
        {
            // Retrieve the matieralIndex
            int32 Face = CurrentIndex / NumIndiciesPerTriangle;
            const int32 MaterialID = Shape.mesh.material_ids[Face];

            // Create a new partition for the mesh
            FMeshPartition& Partition = Data.Partitions.Emplace();
            Partition.BaseVertex = Data.Vertices.Size();
            Partition.StartIndex = Data.Indices.Size();
            
            if (MaterialID >= 0)
            {
                Partition.MaterialIndex = MaterialID;
            }
            
            // Retrieve all vertices/indicies for this partition
            for (; CurrentIndex < IndexCount; ++CurrentIndex)
            {
                // Break if material is not the same
                Face = CurrentIndex / NumIndiciesPerTriangle;
                if (Shape.mesh.material_ids[Face] != MaterialID)
                {
                    break;
                }

                // Normals and texcoords are optional, Positions are required
                const tinyobj::index_t& Index = Shape.mesh.indices[CurrentIndex];
                CHECK(Index.vertex_index >= 0);

                FVertex Vertex;

                const uint32 PositionIndex = NumPositionsPerTriangle * Index.vertex_index;
                Vertex.Position = FVector3(Attributes.vertices[PositionIndex + 0], Attributes.vertices[PositionIndex + 1], Attributes.vertices[PositionIndex + 2]);

                if (Index.normal_index >= 0)
                {
                    const uint32 NormalIndex = NumNormalsPerTriangle * Index.normal_index;
                    Vertex.Normal = FVector3(Attributes.normals[NormalIndex + 0], Attributes.normals[NormalIndex + 1], Attributes.normals[NormalIndex + 2]);
                    Vertex.Normal.Normalize();
                }

                if (Index.texcoord_index >= 0)
                {
                    const uint32 TexCoordIndex = NumTexCoordsPerTriangle * Index.texcoord_index;
                    Vertex.TexCoord = FVector2(Attributes.texcoords[TexCoordIndex + 0], Attributes.texcoords[TexCoordIndex + 1]);
                }

                if (!UniqueVertices.Contains(Vertex))
                {
                    UniqueVertices[Vertex] = static_cast<uint32>(Data.Vertices.Size());
                    Data.Vertices.Add(Vertex);
                }

                Data.Indices.Emplace(UniqueVertices[Vertex]);
            }

            // Set the number of vertices/indices for this partition
            Partition.VertexCount = Data.Vertices.Size() - Partition.BaseVertex;
            Partition.IndexCount  = Data.Indices.Size() - Partition.StartIndex;
            
            // Calculate tangents and create mesh
            FMeshUtilities::CalculateTangents(Data);

            const bool bReverseHandedness = ((Flags & EMeshImportFlags::Default) == EMeshImportFlags::None);
            if (bReverseHandedness)
            {
                FMeshUtilities::ReverseHandedness(Data);
            }

            if (Shape.name.empty())
            {
                Data.Name = FString::CreateFormatted("%s_%d", FilenameWithoutPath.GetCString(), ShapeIndex);
            }
            else
            {
                Data.Name = Shape.name.c_str();
            }
        }

        Scene->Models.Emplace(Data);
        ShapeIndex++;
    }

    Scene->Models.Shrink();
    Scene->Materials.Shrink();

    TSharedRef<FModel> Model = new FModel();
    if (!Model->Init(Scene))
    {
        return nullptr;
    }
    else
    {
        LOG_INFO("[FOBJImporter]: Loaded Model '%s' which contains %d models and %d materials", Filename.GetCString(), Scene->Models.Size(), Scene->Materials.Size());
        return Model;
    }
}

bool FOBJImporter::MatchExtenstion(const FStringView& FileName)
{
    return FileName.EndsWith(".obj", EStringCaseType::NoCase);
}
