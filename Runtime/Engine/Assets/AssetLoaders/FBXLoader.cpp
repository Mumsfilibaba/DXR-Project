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
    
    return FTextureResource2DRef();
}

bool FFBXLoader::LoadFile(const FString& Filename, FSceneData& OutScene, EFBXFlags Flags) noexcept
{
    OutScene.Models.Clear();
    OutScene.Materials.Clear();

    FFileHandleRef File = FPlatformFile::OpenForRead(Filename);
    if (!File)
    {
        LOG_ERROR("[FFBXLoader]: Failed to open '%s'", Filename.GetCString());
        return false;
    }

    // TODO: Utility to read in full file?
    const int32 FileSize = static_cast<int32>(File->Size());
    TArray<ofbx::u8> FileContent(FileSize);

    const int32 NumBytesRead = File->Read(FileContent.Data(), FileSize);
    if (NumBytesRead <= 0)
    {
        LOG_ERROR("[FFBXLoader]: Failed to load '%s'", Filename.GetCString());
        return false;
    }

    ofbx::IScene* FBXScene = ofbx::load(FileContent.Data(), FileSize, (ofbx::u64)ofbx::LoadFlags::NONE);
    if (!FBXScene)
    {
        const CHAR* ErrorString = ofbx::getError();
        LOG_ERROR("[FFBXLoader]: Failed to load content '%s', error '%s'", Filename.GetCString(), ErrorString);
        return false;
    }

    // Estimate sizes to avoid to many allocations
    uint32 MaterialCount = 0;
    for (uint32 MeshIndex = 0; MeshIndex < FBXScene->getMeshCount(); ++MeshIndex)
    {
        const ofbx::Mesh* CurrentMesh = FBXScene->getMesh(MeshIndex);
        MaterialCount += CurrentMesh->getMaterialCount();
    }

    // Array to store indices in when performing triangulation
    TArray<int32> TempIndicies;

    // Unique tables
    TMap<FVertex, uint32, FVertexHasher> UniqueVertices;
    TMap<uint64, uint32> UniqueMaterials;
    UniqueMaterials.reserve(MaterialCount);

    // Estimate resource count
    OutScene.Models.Reserve(FBXScene->getMeshCount());
    OutScene.Materials.Reserve(MaterialCount);

    // Convert data
    const FString Path = ExtractPath(Filename);

    // Get the global settings
    const ofbx::GlobalSettings* GlobalSettings = FBXScene->getGlobalSettings();

    FModelData TempModelData;
    for (uint32 MeshIdx = 0; MeshIdx < FBXScene->getMeshCount(); MeshIdx++)
    {
        const ofbx::Mesh* CurrentMesh = FBXScene->getMesh(MeshIdx);
        for (uint32 MaterialIdx = 0; MaterialIdx < CurrentMesh->getMaterialCount(); MaterialIdx++)
        {
            const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial(MaterialIdx);
            if (UniqueMaterials.count(CurrentMaterial->id) != 0)
            {
                continue;
            }

            FMaterialData MaterialData;
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
            UniqueMaterials[CurrentMaterial->id] = OutScene.Materials.Size();
            OutScene.Materials.Emplace(MaterialData);
        }

        const FMatrix4 Matrix          = ToFloat4x4(CurrentMesh->getGlobalTransform());
        const FMatrix4 GeometricMatrix = ToFloat4x4(CurrentMesh->getGeometricMatrix());
        const FMatrix4 Transform       = Matrix * GeometricMatrix;

        const ofbx::GeometryData& GeometryData = CurrentMesh->getGeometryData();
        ofbx::Vec3Attributes Positions = GeometryData.getPositions();
        ofbx::Vec3Attributes Normals   = GeometryData.getNormals();
        ofbx::Vec3Attributes Tangents  = GeometryData.getTangents();
        ofbx::Vec2Attributes TexCoords = GeometryData.getUVs();

        TempModelData.Mesh.Indices.Reserve(Positions.count);
        TempModelData.Mesh.Vertices.Reserve(Positions.values_count);
        UniqueVertices.reserve(Positions.values_count);
        
        // Go through each mesh partition and add it to the scene as a separate mesh
        for (uint32 PartitionIdx = 0; PartitionIdx < GeometryData.getPartitionCount(); PartitionIdx++)
        {
            // Clear the mesh data every mesh-partition
            TempModelData.Mesh.Clear();
            UniqueVertices.clear();

            ofbx::GeometryPartition Partition = GeometryData.getPartition(PartitionIdx);
            TempIndicies.Resize(Partition.max_polygon_triangles * 3);

            // Go through each polygon and add it to the mesh
            for (uint32 PolygonIdx = 0; PolygonIdx < Partition.polygon_count; ++PolygonIdx)
            {
                // Triangulate this polygon
                const ofbx::GeometryPartition::Polygon& Polygon = Partition.polygons[PolygonIdx];
                const int32 NumIndicies = ofbx::triangulate(GeometryData, Polygon, TempIndicies.Data());

                FVertex TempVertex;
                for (uint32 IndexIdx = 0; IndexIdx < NumIndicies; ++IndexIdx)
                {
                    const uint32 VertexIdx = TempIndicies[IndexIdx];

                    // Position
                    const ofbx::Vec3 OfbxPosition = Positions.get(VertexIdx);
                    const FVector3 Position(OfbxPosition.x, OfbxPosition.y, OfbxPosition.z);
                    TempVertex.Position = Transform.Transform(Position);

                    // Apply the scene scale
                    if ((Flags & EFBXFlags::ApplyScaleFactor) != EFBXFlags::None)
                    {
                        TempVertex.Position *= GlobalSettings->UnitScaleFactor;
                    }

                    // Normal
                    if (Normals.values)
                    {
                        const ofbx::Vec3 OfbxNormal = Normals.get(VertexIdx);
                        const FVector3 Normal(OfbxNormal.x, OfbxNormal.y, OfbxNormal.z);
                        TempVertex.Normal = Transform.TransformNormal(Normal);
                    }

                    // Tangents
                    if (Tangents.values)
                    {
                        const ofbx::Vec3 OfbxTangent = Tangents.get(VertexIdx);
                        const FVector3 Tangent(OfbxTangent.x, OfbxTangent.y, OfbxTangent.z);
                        TempVertex.Tangent = Transform.TransformNormal(Tangent);
                    }

                    // TexCoords
                    if (TexCoords.values)
                    {
                        const ofbx::Vec2 OfbxTexCoord = TexCoords.get(VertexIdx);
                        TempVertex.TexCoord = FVector2(OfbxTexCoord.x, OfbxTexCoord.y);
                    }

                    // Only push unique vertices
                    uint32 UniqueIndex = 0;
                    if (UniqueVertices.count(TempVertex) == 0)
                    {
                        UniqueIndex = static_cast<uint32>(TempModelData.Mesh.Vertices.Size());
                        UniqueVertices[TempVertex] = UniqueIndex;
                        TempModelData.Mesh.Vertices.Add(TempVertex);
                    }
                    else
                    {
                        UniqueIndex = UniqueVertices[TempVertex];
                    }

                    TempModelData.Mesh.Indices.Emplace(UniqueIndex);
                }
            }

            if (!Tangents.values)
            {
                FMeshUtilities::CalculateTangents(TempModelData.Mesh);
            }

            // Convert to left-handed
            if ((Flags & EFBXFlags::EnsureLeftHanded) != EFBXFlags::None)
            {
                if (GlobalSettings->CoordAxis == ofbx::CoordSystem_RightHanded)
                {
                    FMeshUtilities::ReverseHandedness(TempModelData.Mesh);
                }
            }

            // Add material index to the mesh
            TempModelData.MaterialIndex = INVALID_MATERIAL_INDEX;
            if (PartitionIdx < CurrentMesh->getMaterialCount())
            {
                const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial(PartitionIdx);
                if (UniqueMaterials.count(CurrentMaterial->id) != 0)
                {
                    TempModelData.MaterialIndex = UniqueMaterials[CurrentMaterial->id];
                }
            }

            if (TempModelData.MaterialIndex == INVALID_MATERIAL_INDEX)
            {
                LOG_WARNING("Mesh '%s' has no material", CurrentMesh->name);
            }

            // Add the mesh to our scene
            if (TempModelData.Mesh.Hasdata())
            {
                LOG_INFO("Loaded Mesh '%s'", CurrentMesh->name);
                TempModelData.Name = CurrentMesh->name;
                OutScene.Models.Emplace(TempModelData);
            }
            else
            {
                LOG_WARNING("Tried to load mesh without any data");
            }
        }
    }

    OutScene.Models.Shrink();
    OutScene.Materials.Shrink();

    FBXScene->destroy();
    return true;
}
