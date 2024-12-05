#include "Core/Math/Matrix4.h"
#include "Core/Containers/Map.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"
#include "Engine/Assets/VertexFormat.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetImporters/FBXImporter.h"

#include <ofbx.h>

#define INVALID_MATERIAL_INDEX (-1)

static FString ExtractPath(const FString& FullFilePath)
{
    auto Pos = FullFilePath.FindLastChar('/');
    if (Pos != FString::InvalidIndex)
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
            Result.M[y][x] = static_cast<float>(Matrix.m[Index]);
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
        Filename.ReplaceAll('\\', '/');

        return StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(Filename, false));
    }

    return FTexture2DRef();
}

bool FFBXImporter::ImportFromFile(const FStringView& InFilename, EMeshImportFlags InFlags, FModelCreateInfo& OutModelInfo)
{
    const FString Filename = FString(InFilename);

    FFileHandleRef File = FPlatformFile::OpenForRead(Filename);
    if (!File)
    {
        LOG_ERROR("[FFBXImporter]: Failed to open '%s'", *Filename);
        return false;
    }

    // TODO: Utility to read in full file?
    const int32 FileSize = static_cast<int32>(File->Size());
    TArray<ofbx::u8> FileContent(FileSize);

    const int32 NumBytesRead = File->Read(FileContent.Data(), FileSize);
    if (NumBytesRead <= 0)
    {
        LOG_ERROR("[FFBXImporter]: Failed to load '%s'", *Filename);
        return false;
    }

    ofbx::IScene* FBXScene = ofbx::load(FileContent.Data(), FileSize, static_cast<ofbx::u64>(ofbx::LoadFlags::NONE));
    if (!FBXScene)
    {
        const CHAR* ErrorString = ofbx::getError();
        LOG_ERROR("[FFBXImporter]: Failed to load content '%s', error '%s'", *Filename, ErrorString);
        return false;
    }

    EFBXFlags FBXFlags = EFBXFlags::None;
    if ((InFlags & EMeshImportFlags::ApplyScaleFactor) != EMeshImportFlags::None)
    {
        FBXFlags |= EFBXFlags::ApplyScaleFactor;
    }

    if ((InFlags & EMeshImportFlags::ForceLeftHanded) != EMeshImportFlags::None)
    {
        FBXFlags |= EFBXFlags::ForceLeftHanded;
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
    OutModelInfo.Meshes.Reserve(FBXScene->getMeshCount());
    OutModelInfo.Materials.Reserve(MaterialCount);

    // Convert data
    const FString Path = ExtractPath(Filename);

    // Get the global settings
    const ofbx::GlobalSettings* GlobalSettings = FBXScene->getGlobalSettings();

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

            LOG_INFO("[FFBXImporter] Loading Material '%s'", CurrentMaterial->name);

            FMaterialCreateInfo MaterialCreateInfo;
            MaterialCreateInfo.Name = CurrentMaterial->name;

            MaterialCreateInfo.Textures[EMaterialTexture::Diffuse]          = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::DIFFUSE);
            MaterialCreateInfo.Textures[EMaterialTexture::Normal]           = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::NORMAL);
            MaterialCreateInfo.Textures[EMaterialTexture::Specular]         = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::SPECULAR);
            MaterialCreateInfo.Textures[EMaterialTexture::Emissive]         = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::EMISSIVE);
            MaterialCreateInfo.Textures[EMaterialTexture::AmbientOcclusion] = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::AMBIENT);

            MaterialCreateInfo.Diffuse       = FVector3(CurrentMaterial->getDiffuseColor().r, CurrentMaterial->getDiffuseColor().g, CurrentMaterial->getDiffuseColor().b);
            MaterialCreateInfo.AmbientFactor = 1.0f; // CurrentMaterial->getSpecularColor().r;
            MaterialCreateInfo.Roughness     = 1.0f; // CurrentMaterial->getSpecularColor().g;
            MaterialCreateInfo.Metallic      = 1.0f; // CurrentMaterial->getSpecularColor().b;

            // TODO: Other material properties
            UniqueMaterials[CurrentMaterial->id] = OutModelInfo.Materials.Size();
            OutModelInfo.Materials.Add(Move(MaterialCreateInfo));
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
        
        FMeshCreateInfo MeshCreateInfo;
        MeshCreateInfo.Indices.Reserve(Positions.count);
        MeshCreateInfo.Vertices.Reserve(Positions.values_count);
        MeshCreateInfo.SubMeshes.Resize(PartitionCount);

        // Clear the mesh data to start a new mesh
        UniqueVertices.Reserve(Positions.values_count);
        UniqueVertices.Clear();

        // Go through each mesh partition and add it to the scene as a separate mesh
        for (int32 PartitionIdx = 0; PartitionIdx < PartitionCount; PartitionIdx++)
        {
            constexpr int32 NumIndiciesPerTriangle = 3;
            ofbx::GeometryPartition FbxPartition = GeometryData.getPartition(PartitionIdx);
            PartitionIndicies.Resize(FbxPartition.max_polygon_triangles * NumIndiciesPerTriangle);

            // Create a new partition for the mesh
            FSubMeshInfo& SubMeshInfo = MeshCreateInfo.SubMeshes[PartitionIdx];
            SubMeshInfo.BaseVertex = MeshCreateInfo.Vertices.Size();
            SubMeshInfo.StartIndex = MeshCreateInfo.Indices.Size();

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
                    if ((FBXFlags & EFBXFlags::ApplyScaleFactor) != EFBXFlags::None)
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
                    if (uint32* ExistingIndex = UniqueVertices.Find(Vertex))
                    {
                        UniqueIndex = *ExistingIndex;
                    }
                    else
                    {
                        UniqueIndex = static_cast<uint32>(MeshCreateInfo.Vertices.Size());
                        UniqueVertices[Vertex] = UniqueIndex;
                        MeshCreateInfo.Vertices.Add(Vertex);
                    }

                    MeshCreateInfo.Indices.Emplace(UniqueIndex);
                }
            }

            // Set the number of vertices/indices for this partition
            SubMeshInfo.VertexCount = MeshCreateInfo.Vertices.Size() - SubMeshInfo.BaseVertex;
            SubMeshInfo.IndexCount  = MeshCreateInfo.Indices.Size() - SubMeshInfo.StartIndex;

            // Add material index to the mesh
            SubMeshInfo.MaterialIndex = INVALID_MATERIAL_INDEX;
            if (PartitionIdx < CurrentMesh->getMaterialCount())
            {
                const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial(PartitionIdx);
                if (uint32* ExistingMaterialIndex = UniqueMaterials.Find(CurrentMaterial->id))
                {
                    SubMeshInfo.MaterialIndex = *ExistingMaterialIndex;
                }
            }

            if (SubMeshInfo.MaterialIndex == INVALID_MATERIAL_INDEX)
            {
                LOG_WARNING("[FFBXImporter] Partition in Mesh '%s' has no material", CurrentMesh->name);
            }
        }

        // If there are no tangents, then we calculate them
        if (!Tangents.values)
        {
            MeshCreateInfo.CalculateTangents();
        }

        // Convert to left-handed
        if ((FBXFlags & EFBXFlags::ForceLeftHanded) != EFBXFlags::None)
        {
            if (GlobalSettings->CoordAxis == ofbx::CoordSystem_RightHanded)
            {
                MeshCreateInfo.ReverseHandedness();
            }
        }

        // Add the mesh to our scene
        if (!MeshCreateInfo.Vertices.IsEmpty())
        {
            LOG_INFO("[FFBXImporter] Loaded Mesh '%s'", CurrentMesh->name);
            MeshCreateInfo.Name = CurrentMesh->name;
            OutModelInfo.Meshes.Add(Move(MeshCreateInfo));
        }
        else
        {
            LOG_WARNING("[FFBXImporter] Tried to load mesh without any data");
        }
    }

    OutModelInfo.Meshes.Shrink();
    OutModelInfo.Materials.Shrink();

    FBXScene->destroy();

    LOG_INFO("[FFBXImporter]: Loaded Model '%s' which contains %d meshes and %d materials", *Filename, OutModelInfo.Meshes.Size(), OutModelInfo.Materials.Size());
    return true;
}

bool FFBXImporter::MatchExtenstion(const FStringView& FileName)
{
    return FileName.EndsWith(".fbx", EStringCaseType::NoCase);
}
