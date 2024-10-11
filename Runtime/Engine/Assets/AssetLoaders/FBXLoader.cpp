#include "FBXLoader.h"
#include "Engine/Assets/VertexFormat.h"
#include "Engine/Assets/MeshUtilities.h"
#include "Engine/Assets/AssetManager.h"
#include "Core/Math/Matrix4.h"
#include "Core/Containers/Map.h"
#include "Core/Utilities/StringUtilities.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"

#include <ofbx.h>

#define INVALID_MATERIAL_INDEX (-1)

static FString ExtractPath(const FString& FullFilePath)
{
    auto Pos = FullFilePath.FindLastChar('/');
    if (Pos != FString::INVALID_INDEX)
    {
        return FullFilePath.SubString(0, Pos);
    }
    else
    {
        return FullFilePath;
    }
}

static FMatrix4 ToFloat4x4(const ofbx::DMatrix& Matrix)
{
    FMatrix4 Result;
    for (uint32 y = 0; y < 4; y++)
    {
        for (uint32 x = 0; x < 4; x++)
        {
            const uint32 Index = y * 4 + x;
            Result.f[y][x] = static_cast<float>(Matrix.m[Index]);
        }
    }

    return Result;
}

static auto LoadMaterialTexture(const FString& Path, const ofbx::Material* Material, ofbx::Texture::TextureType Type)
{
    const ofbx::Texture* MaterialTexture = Material->getTexture(Type);
    if (MaterialTexture)
    {
        CHAR StringBuffer[256];
        MaterialTexture->getRelativeFileName().toString(StringBuffer);

        // Make sure that correct slashes are used
        FString Filename = Path + '/' + StringBuffer;
        ConvertBackslashes(Filename);

        return StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(Filename, false));
    }
    
    return FTexture2DRef();
}

TSharedPtr<FImportedModel> FFBXLoader::LoadFile(const FString& Filename, EFBXFlags Flags) noexcept
{
    FFileHandleRef File = FPlatformFile::OpenForRead(Filename);
    if (!File)
    {
        LOG_ERROR("[FFBXLoader]: Failed to open '%s'", Filename.GetCString());
        return nullptr;
    }

    // TODO: Utility to read in full file?
    const int32 FileSize = static_cast<int32>(File->Size());
    TArray<ofbx::u8> FileContent(FileSize);

    const int32 NumBytesRead = File->Read(FileContent.Data(), FileSize);
    if (NumBytesRead <= 0)
    {
        LOG_ERROR("[FFBXLoader]: Failed to load '%s'", Filename.GetCString());
        return nullptr;
    }

    ofbx::IScene* FBXScene = ofbx::load(FileContent.Data(), FileSize, (ofbx::u64)ofbx::LoadFlags::NONE);
    if (!FBXScene)
    {
        const CHAR* ErrorString = ofbx::getError();
        LOG_ERROR("[FFBXLoader]: Failed to load content '%s', error '%s'", Filename.GetCString(), ErrorString);
        return nullptr;
    }

    // Estimate sizes to avoid to many allocations
    uint32 MaterialCount = 0;
    for (int32 MeshIndex = 0; MeshIndex < FBXScene->getMeshCount(); ++MeshIndex)
    {
        const ofbx::Mesh* CurrentMesh = FBXScene->getMesh(MeshIndex);
        MaterialCount += CurrentMesh->getMaterialCount();
    }

    // Array to store indices in when performing triangulation
    TArray<int32> PartitionIndicies;

    // Unique tables
    TMap<FVertex, uint32> UniqueVertices;
    TMap<uint64, uint32>  UniqueMaterials;
    UniqueMaterials.Reserve(MaterialCount);

    // Estimate resource count
    TSharedPtr<FImportedModel> Scene = MakeSharedPtr<FImportedModel>();
    Scene->Models.Reserve(FBXScene->getMeshCount());
    Scene->Materials.Reserve(MaterialCount);

    // Convert data
    const FString Path = ExtractPath(Filename);

    // Get the global settings
    const ofbx::GlobalSettings* GlobalSettings = FBXScene->getGlobalSettings();

    FMeshData ModelData;
    for (int32 MeshIdx = 0; MeshIdx < FBXScene->getMeshCount(); MeshIdx++)
    {
        const ofbx::Mesh* CurrentMesh = FBXScene->getMesh(MeshIdx);
        for (int32 MaterialIdx = 0; MaterialIdx < CurrentMesh->getMaterialCount(); MaterialIdx++)
        {
            const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial(MaterialIdx);
            if (UniqueMaterials.Contains(CurrentMaterial->id))
            {
                continue;
            }

            LOG_INFO("Loading Material '%s'", CurrentMaterial->name);

            FImportedMaterial MaterialData;
            MaterialData.Name = CurrentMaterial->name;

            MaterialData.DiffuseTexture  = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::DIFFUSE);
            MaterialData.NormalTexture   = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::NORMAL);
            MaterialData.SpecularTexture = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::SPECULAR);
            MaterialData.EmissiveTexture = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::EMISSIVE);
            MaterialData.AOTexture       = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::AMBIENT);

            MaterialData.Diffuse   = FVector3(CurrentMaterial->getDiffuseColor().r, CurrentMaterial->getDiffuseColor().g, CurrentMaterial->getDiffuseColor().b);
            MaterialData.AO        = 1.0f; // CurrentMaterial->getSpecularColor().r;
            MaterialData.Roughness = 1.0f; // CurrentMaterial->getSpecularColor().g;
            MaterialData.Metallic  = 1.0f; // CurrentMaterial->getSpecularColor().b;

            // TODO: Other material properties
            UniqueMaterials[CurrentMaterial->id] = Scene->Materials.Size();
            Scene->Materials.Emplace(MaterialData);
        }

        const FMatrix4 Matrix          = ToFloat4x4(CurrentMesh->getGlobalTransform());
        const FMatrix4 GeometricMatrix = ToFloat4x4(CurrentMesh->getGeometricMatrix());
        const FMatrix4 Transform       = Matrix * GeometricMatrix;

        const ofbx::GeometryData& GeometryData = CurrentMesh->getGeometryData();
        ofbx::Vec3Attributes Positions = GeometryData.getPositions();
        ofbx::Vec3Attributes Normals   = GeometryData.getNormals();
        ofbx::Vec3Attributes Tangents  = GeometryData.getTangents();
        ofbx::Vec2Attributes TexCoords = GeometryData.getUVs();

        const int32 PartitionCount = GeometryData.getPartitionCount();
        ModelData.Indices.Reserve(Positions.count);
        ModelData.Vertices.Reserve(Positions.values_count);
        ModelData.Partitions.Reserve(PartitionCount);
        UniqueVertices.Reserve(Positions.values_count);
        
        // Clear the mesh data to start a new mesh
        ModelData.Clear();
        UniqueVertices.Clear();

        // Go through each mesh partition and add it to the scene as a separate mesh
        for (int32 PartitionIdx = 0; PartitionIdx < PartitionCount; PartitionIdx++)
        {
            constexpr int32 NumIndiciesPerTriangle = 3;
            ofbx::GeometryPartition FbxPartition = GeometryData.getPartition(PartitionIdx);
            PartitionIndicies.Resize(FbxPartition.max_polygon_triangles * NumIndiciesPerTriangle);

            // Create a new partition for the mesh
            FMeshPartition& Partition = ModelData.Partitions.Emplace();
            Partition.BaseVertex = ModelData.Vertices.Size();
            Partition.StartIndex = ModelData.Indices.Size();
            
            // Go through each polygon and add it to the mesh
            for (int32 PolygonIdx = 0; PolygonIdx < FbxPartition.polygon_count; ++PolygonIdx)
            {
                // Triangulate this polygon
                const ofbx::GeometryPartition::Polygon& Polygon = FbxPartition.polygons[PolygonIdx];
                const int32 NumIndicies = ofbx::triangulate(GeometryData, Polygon, PartitionIndicies.Data());

                FVertex Vertex;
                for (int32 IndexIdx = 0; IndexIdx < NumIndicies; ++IndexIdx)
                {
                    const int32 VertexIdx = PartitionIndicies[IndexIdx];

                    // Position
                    const ofbx::Vec3 OfbxPosition = Positions.get(VertexIdx);
                    const FVector3 Position(OfbxPosition.x, OfbxPosition.y, OfbxPosition.z);
                    Vertex.Position = Transform.Transform(Position);

                    // Apply the scene scale
                    if ((Flags & EFBXFlags::ApplyScaleFactor) != EFBXFlags::None)
                    {
                        Vertex.Position *= GlobalSettings->UnitScaleFactor;
                    }

                    // Normal
                    if (Normals.values)
                    {
                        const ofbx::Vec3 OfbxNormal = Normals.get(VertexIdx);
                        const FVector3 Normal(OfbxNormal.x, OfbxNormal.y, OfbxNormal.z);
                        Vertex.Normal = Transform.TransformNormal(Normal);
                    }

                    // Tangents
                    if (Tangents.values)
                    {
                        const ofbx::Vec3 OfbxTangent = Tangents.get(VertexIdx);
                        const FVector3 Tangent(OfbxTangent.x, OfbxTangent.y, OfbxTangent.z);
                        Vertex.Tangent = Transform.TransformNormal(Tangent);
                    }

                    // TexCoords
                    if (TexCoords.values)
                    {
                        const ofbx::Vec2 OfbxTexCoord = TexCoords.get(VertexIdx);
                        Vertex.TexCoord = FVector2(OfbxTexCoord.x, OfbxTexCoord.y);
                    }

                    // Only push unique vertices
                    uint32 UniqueIndex = 0;
                    if (!UniqueVertices.Contains(Vertex))
                    {
                        UniqueIndex = static_cast<uint32>(ModelData.Vertices.Size());
                        UniqueVertices[Vertex] = UniqueIndex;
                        ModelData.Vertices.Add(Vertex);
                    }
                    else
                    {
                        UniqueIndex = UniqueVertices[Vertex];
                    }

                    ModelData.Indices.Emplace(UniqueIndex);
                }
            }

            // Set the number of vertices/indices for this partition
            Partition.VertexCount = ModelData.Vertices.Size() - Partition.BaseVertex;
            Partition.IndexCount  = ModelData.Indices.Size() - Partition.StartIndex;

            // Add material index to the mesh
            Partition.MaterialIndex = INVALID_MATERIAL_INDEX;
            if (PartitionIdx < CurrentMesh->getMaterialCount())
            {
                const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial(PartitionIdx);
                if (UniqueMaterials.Contains(CurrentMaterial->id))
                {
                    Partition.MaterialIndex = UniqueMaterials[CurrentMaterial->id];
                }
            }

            if (Partition.MaterialIndex == INVALID_MATERIAL_INDEX)
            {
                LOG_WARNING("Partition in Mesh '%s' has no material", CurrentMesh->name);
            }
        }
        
        // If there are no tangents, then we calculate them
        if (!Tangents.values)
        {
            FMeshUtilities::CalculateTangents(ModelData);
        }
        
        // Convert to left-handed
        if ((Flags & EFBXFlags::ForceLeftHanded) != EFBXFlags::None)
        {
            if (GlobalSettings->CoordAxis == ofbx::CoordSystem_RightHanded)
            {
                FMeshUtilities::ReverseHandedness(ModelData);
            }
        }

        // Add the mesh to our scene
        if (ModelData.Hasdata())
        {
            LOG_INFO("Loaded Mesh '%s'", CurrentMesh->name);
            ModelData.Name = CurrentMesh->name;
            Scene->Models.Emplace(ModelData);
        }
        else
        {
            LOG_WARNING("Tried to load mesh without any data");
        }
    }

    Scene->Models.Shrink();
    Scene->Materials.Shrink();

    FBXScene->destroy();
    return Scene;
}
