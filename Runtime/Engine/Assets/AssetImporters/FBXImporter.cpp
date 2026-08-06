#include "Core/Math/Matrix4.h"
#include "Core/Containers/Map.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"
#include "Engine/Assets/VertexFormat.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetImporters/FBXImporter.h"

#include <ofbx.h>

#define INVALID_MATERIAL_INDEX (-1)

static String ExtractPath(const String& FullFilePath)
{
    auto Pos = FullFilePath.FindLastChar('/');
    if (Pos != String::InvalidIndex)
    {
        return FullFilePath.SubString(0, Pos);
    }
    else
    {
        return FullFilePath;
    }
}

static Matrix4 FBXConvertMatrix(const ofbx::DMatrix& Matrix)
{
    Matrix4 Result;
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

#if 0 // Currently unused
static bool DoesFlipHandness(const Matrix4& Matrix)
{
    Vector3 X(1.0f, 0.0f, 0.0f);
    X = Matrix.TransformNormal(X);
    
    Vector3 Y(0.0f, 1.0f, 0.0f);
    Y = Matrix.TransformNormal(Y);

    Vector3 Z = Matrix.GetInverse().TransformCoord(X.CrossProduct(Y));
    return Z.Z < 0.0f;
}
#endif

static auto LoadMaterialTexture(const String& Path, const ofbx::Material* Material, ofbx::Texture::TextureType Type)
{
    const ofbx::Texture* MaterialTexture = Material->getTexture(Type);
    if (MaterialTexture)
    {
        CHAR StringBuffer[256];
        MaterialTexture->getRelativeFileName().toString(StringBuffer);

        // Make sure that correct slashes are used
        String Filename = Path + '/' + StringBuffer;
        Filename.ReplaceAll('\\', '/');

        return StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(Filename, false));
    }

    return FTexture2DRef();
}

bool FFBXImporter::ImportFromFile(const StringView& InFilename, EMeshImportFlags InFlags, FModelData& OutModelData)
{
    const String Filename = String(InFilename);

    TFileRef<IPlatformFile> File = FPlatformFile::OpenForRead(Filename);
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
    TMap<uint64, uint32>  UniqueMaterials;
    TMap<FSourceVertex, uint32> UniqueVertices;
    UniqueMaterials.Reserve(MaterialCount);

    // Estimate resource count
    OutModelData.Meshes.Reserve(FBXScene->getMeshCount());
    OutModelData.Materials.Reserve(MaterialCount);

    // Convert data
    const String Path = ExtractPath(Filename);

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

            FMaterialData MaterialData;
            MaterialData.Name = CurrentMaterial->name;

            MaterialData.Textures[EMaterialTexture::Diffuse]          = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::DIFFUSE);
            MaterialData.Textures[EMaterialTexture::Normal]           = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::NORMAL);
            MaterialData.Textures[EMaterialTexture::Specular]         = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::SPECULAR);
            MaterialData.Textures[EMaterialTexture::Emissive]         = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::EMISSIVE);
            MaterialData.Textures[EMaterialTexture::AmbientOcclusion] = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::AMBIENT);

            MaterialData.Diffuse       = Vector3(CurrentMaterial->getDiffuseColor().r, CurrentMaterial->getDiffuseColor().g, CurrentMaterial->getDiffuseColor().b);
            MaterialData.AmbientFactor = 1.0f; // CurrentMaterial->getSpecularColor().r;
            MaterialData.Roughness     = 1.0f; // CurrentMaterial->getSpecularColor().g;
            MaterialData.Metallic      = 1.0f; // CurrentMaterial->getSpecularColor().b;

            // TODO: Other material properties
            UniqueMaterials[CurrentMaterial->id] = OutModelData.Materials.Size();
            OutModelData.Materials.Add(Move(MaterialData));
        }

        const bool bApplyScaleFactor = (InFlags & EMeshImportFlags::ApplyScaleFactor) != EMeshImportFlags::None;

        const Matrix4 ScaleMatrix     = Matrix4::Scale(bApplyScaleFactor ? GlobalSettings->UnitScaleFactor : 1.0f);
        const Matrix4 GlobalTransform = FBXConvertMatrix(CurrentMesh->getGlobalTransform());
        const Matrix4 GeometricMatrix = FBXConvertMatrix(CurrentMesh->getGeometricMatrix());
        const Matrix4 Transform       = GlobalTransform * GeometricMatrix * ScaleMatrix;

        const ofbx::GeometryData& GeometryData = CurrentMesh->getGeometryData();
        ofbx::Vec3Attributes Positions = GeometryData.getPositions();
        ofbx::Vec3Attributes Normals   = GeometryData.getNormals();
        ofbx::Vec3Attributes Tangents  = GeometryData.getTangents();
        ofbx::Vec2Attributes TexCoords = GeometryData.getUVs();

        const int32 PartitionCount = GeometryData.getPartitionCount();

        FMeshData MeshData;
        MeshData.Indices.Reserve(Positions.count);
        MeshData.Vertices.Reserve(Positions.values_count);
        MeshData.SubMeshes.Resize(PartitionCount);

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
            FSubMesh& SubMesh = MeshData.SubMeshes[PartitionIdx];
            SubMesh.BaseVertex = MeshData.Vertices.Size();
            SubMesh.StartIndex = MeshData.Indices.Size();

            // Go through each polygon and add it to the mesh
            for (int32 PolygonIdx = 0; PolygonIdx < FbxPartition.polygon_count; ++PolygonIdx)
            {
                // Triangulate this polygon
                const ofbx::GeometryPartition::Polygon& Polygon = FbxPartition.polygons[PolygonIdx];
                const int32 NumIndicies = ofbx::triangulate(GeometryData, Polygon, PartitionIndicies.Data());

                FSourceVertex Vertex;
                for (int32 IndexIdx = 0; IndexIdx < NumIndicies; ++IndexIdx)
                {
                    const int32 VertexIdx = PartitionIndicies[IndexIdx];

                    // Position
                    const ofbx::Vec3 OfbxPosition = Positions.get(VertexIdx);
                    const Vector3 Position(OfbxPosition.x, OfbxPosition.y, OfbxPosition.z);
                    Vertex.Position = Transform.Transform(Position);

                    // Normal
                    if (Normals.values)
                    {
                        const ofbx::Vec3 OfbxNormal = Normals.get(VertexIdx);
                        const Vector3 Normal(OfbxNormal.x, OfbxNormal.y, OfbxNormal.z);
                        Vertex.Normal = Transform.TransformNormal(Normal);
                    }

                    // Tangents
                    if (Tangents.values)
                    {
                        const ofbx::Vec3 OfbxTangent = Tangents.get(VertexIdx);
                        const Vector3 Tangent(OfbxTangent.x, OfbxTangent.y, OfbxTangent.z);
                        Vertex.Tangent = Transform.TransformNormal(Tangent);
                    }

                    // TexCoords
                    if (TexCoords.values)
                    {
                        // We need to correct UVs (I assume since DirectX coordinate system)
                        const ofbx::Vec2 OfbxTexCoord = TexCoords.get(VertexIdx);
                        Vertex.TexCoord = Vector2(OfbxTexCoord.x, 1.0f - OfbxTexCoord.y);
                    }

                    // Only push unique vertices
                    uint32 UniqueIndex = 0;
                    if (uint32* ExistingIndex = UniqueVertices.Find(Vertex))
                    {
                        UniqueIndex = *ExistingIndex;
                    }
                    else
                    {
                        UniqueIndex = static_cast<uint32>(MeshData.Vertices.Size());
                        UniqueVertices[Vertex] = UniqueIndex;
                        MeshData.Vertices.Add(Vertex);
                    }

                    MeshData.Indices.Emplace(UniqueIndex);
                }
            }

            // Set the number of vertices/indices for this partition
            SubMesh.VertexCount = MeshData.Vertices.Size() - SubMesh.BaseVertex;
            SubMesh.IndexCount  = MeshData.Indices.Size() - SubMesh.StartIndex;

            // Add material index to the mesh
            SubMesh.MaterialIndex = INVALID_MATERIAL_INDEX;
            if (PartitionIdx < CurrentMesh->getMaterialCount())
            {
                const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial(PartitionIdx);
                if (uint32* ExistingMaterialIndex = UniqueMaterials.Find(CurrentMaterial->id))
                {
                    SubMesh.MaterialIndex = *ExistingMaterialIndex;
                }
            }

            if (SubMesh.MaterialIndex == INVALID_MATERIAL_INDEX)
            {
                LOG_WARNING("[FFBXImporter] Partition in Mesh '%s' has no material", CurrentMesh->name);
            }
        }

        // Convert to left-handed
        if ((InFlags & EMeshImportFlags::ForceLeftHanded) != EMeshImportFlags::None)
        {
            if (GlobalSettings->CoordAxis == ofbx::CoordSystem_RightHanded)
            {
                MeshData.ReverseHandedness();
            }
        }

        if ((InFlags & EMeshImportFlags::InvertAxisX) != EMeshImportFlags::None)
        {
            MeshData.InvertAxisX();
        }

        // If there are no tangents, then we calculate them
        const bool bRecalculateTangents = (InFlags & EMeshImportFlags::RecalculateTangents) != EMeshImportFlags::None;
        if (!Tangents.values || bRecalculateTangents)
        {
            MeshData.CalculateTangents();
        }
        else
        {
            MeshData.CalculateTangentSigns();
        }

        // Add the mesh to our scene
        if (!MeshData.Vertices.IsEmpty())
        {
            LOG_INFO("[FFBXImporter] Loaded Mesh '%s'", CurrentMesh->name);
            MeshData.Name = CurrentMesh->name;
            OutModelData.Meshes.Add(Move(MeshData));
        }
        else
        {
            LOG_WARNING("[FFBXImporter] Tried to load mesh without any data");
        }
    }

    OutModelData.Meshes.Shrink();
    OutModelData.Materials.Shrink();

    FBXScene->destroy();

    LOG_INFO("[FFBXImporter]: Loaded Model '%s' which contains %d meshes and %d materials", *Filename, OutModelData.Meshes.Size(), OutModelData.Materials.Size());
    return true;
}

bool FFBXImporter::MatchExtenstion(const StringView& FileName)
{
    return FileName.EndsWith(".fbx", EStringCaseType::NoCase);
}
