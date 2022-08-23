#include "FBXLoader.h"

#include "Engine/Assets/VertexFormat.h"
#include "Engine/Assets/MeshUtilities.h"

#include "Core/Math/Matrix4.h"
#include "Core/Containers/Map.h"
#include "Core/Utilities/StringUtilities.h"
#include "Core/Misc/OutputDeviceLogger.h"

#include <ofbx.h>

#define INVALID_MATERIAL_INDEX (-1)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FFBXLoader

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

static FMatrix4 ToFloat4x4(const ofbx::Matrix& Matrix)
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

#if 0
static void GetMatrix(const ofbx::Object* Mesh, FMatrix4& OutMatrix)
{
    if (Mesh)
    {
        FMatrix4 Matrix;
        GetMatrix(Mesh->getParent(), Matrix);

        ofbx::Vec3 Scaling = Mesh->getLocalScaling();
        ofbx::Vec3 Rotation = Mesh->getLocalRotation();
        ofbx::Vec3 Translation = Mesh->getLocalTranslation();

        FMatrix4 LocalMatrix = ToFloat4x4(Mesh->evalLocal(Translation, Rotation, Scaling));
        OutMatrix = Matrix * LocalMatrix;
    }
    else
    {
        OutMatrix = FMatrix4::Identity();
    }
}
#endif

#if 0
static FImage2DPtr LoadMaterialTexture(const FString& Path, const ofbx::Material* Material, ofbx::Texture::TextureType Type)
{
#if 0
    const ofbx::Texture* MaterialTexture = Material->getTexture(Type);
    if (MaterialTexture)
    {
        // Non-static buffer to support multi threading
        char StringBuffer[256];
        MaterialTexture->getRelativeFileName().toString(StringBuffer);

        // Make sure that correct slashes are used
        String Filename = StringBuffer;
        ConvertBackslashes(Filename);

        FImage2DPtr Texture = MakeShared<FImage2D>();
        return Texture;
    }
    else
#else
    UNREFERENCED_VARIABLE(Path);
    UNREFERENCED_VARIABLE(Material);
    UNREFERENCED_VARIABLE(Type);
#endif
    {
        return FImage2DPtr();
    }
}
#endif

bool FFBXLoader::LoadFile(const FString& Filename, FSceneData& OutScene, uint32 Flags) noexcept
{
    OutScene.Models.Clear();
    OutScene.Materials.Clear();

    FILE* File = fopen(Filename.GetCString(), "rb");
    if (!File)
    {
        LOG_ERROR("[FFBXLoader]: Failed to open '%s'", Filename.GetCString());
        return false;
    }

    // TODO: Utility to read in full file?
    fseek(File, 0, SEEK_END);
    uint32 FileSize = (uint32)ftell(File);
    rewind(File);

    TArray<ofbx::u8> FileContent(FileSize);
    uint32 SizeInBytes = FileContent.SizeInBytes();
    UNREFERENCED_VARIABLE(SizeInBytes);

    ofbx::u8* Bytes = FileContent.GetData();

    const uint32 ChunkSize = 1024;

    uint32 NumBytesRead = 0;
    while (NumBytesRead < FileSize)
    {
        NumBytesRead += (uint32)fread(Bytes, 1, ChunkSize, File);
        Bytes += ChunkSize;
    }

    if (NumBytesRead != FileSize)
    {
        LOG_ERROR("[FFBXLoader]: Failed to load '%s'", Filename.GetCString());
        return false;
    }

    Bytes = FileContent.GetData();

    ofbx::IScene* FBXScene = ofbx::load(Bytes, FileSize, (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
    if (!FBXScene)
    {
        LOG_ERROR("[FMeshFactory]: Failed to load content '%s'", Filename.GetCString());
        return false;
    }

    const ofbx::GlobalSettings* Settings = FBXScene->getGlobalSettings();
    const float UnitScaleFactorRecip     = Settings->UnitScaleFactor;

    // Unique tables
    TMap<FVertex, uint32, FVertexHasher>  UniqueVertices;
    TMap<const ofbx::Material*, uint32> UniqueMaterials;

    FString Path = ExtractPath(Filename);

    FModelData Data;

    uint32 MeshCount = FBXScene->getMeshCount();
    for (uint32 i = 0; i < MeshCount; i++)
    {
        const ofbx::Mesh*     CurrentMesh = FBXScene->getMesh(i);
        const ofbx::Geometry* CurrentGeom = CurrentMesh->getGeometry();

        uint32 MaterialCount = CurrentMesh->getMaterialCount();
        for (uint32 j = 0; j < MaterialCount; j++)
        {
            const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial(j);
            if (UniqueMaterials.count(CurrentMaterial) != 0)
            {
                continue;
            }

            FMaterialData MaterialData;
            // MaterialData.DiffuseTexture  = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::DIFFUSE);
            // MaterialData.NormalTexture   = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::NORMAL);
            // MaterialData.SpecularTexture = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::SPECULAR);
            // MaterialData.EmissiveTexture = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::EMISSIVE);
            // MaterialData.AOTexture       = LoadMaterialTexture(Path, CurrentMaterial, ofbx::Texture::TextureType::AMBIENT);

            MaterialData.Diffuse   = FVector3(CurrentMaterial->getDiffuseColor().r, CurrentMaterial->getDiffuseColor().g, CurrentMaterial->getDiffuseColor().b);
            MaterialData.AO        = 1.0f; // CurrentMaterial->getSpecularColor().r;
            MaterialData.Roughness = 1.0f; // CurrentMaterial->getSpecularColor().g;
            MaterialData.Metallic  = 1.0f; // CurrentMaterial->getSpecularColor().b;

            //TODO: Other material properties
            UniqueMaterials[CurrentMaterial] = OutScene.Materials.GetSize();
            OutScene.Materials.Emplace(MaterialData);
        }

        int32 VertexCount = (int32)CurrentGeom->getVertexCount();
        int32 IndexCount  = (int32)CurrentGeom->getIndexCount();
        Data.Mesh.Indices.Reserve(IndexCount);
        Data.Mesh.Vertices.Reserve(VertexCount);
        UniqueVertices.reserve(VertexCount);

        const auto* Materials = CurrentGeom->getMaterials();

        const ofbx::Vec3* Vertices = CurrentGeom->getVertices();
        Check(Vertices != nullptr);

        const ofbx::Vec3* Normals = CurrentGeom->getNormals();
        Check(Normals != nullptr);

        const ofbx::Vec2* TexCoords = CurrentGeom->getUVs(0);
        Check(TexCoords != nullptr);

        const ofbx::Vec3* Tangents = CurrentGeom->getTangents();

        FMatrix4 Matrix          = ToFloat4x4(CurrentMesh->getGlobalTransform());
        FMatrix4 GeometricMatrix = ToFloat4x4(CurrentMesh->getGeometricMatrix());
        FMatrix4 Transform       = Matrix * GeometricMatrix;

        int32 CurrentIndex      = 0;
        int32 MaterialIndex     = Materials ? Materials[0] : INVALID_MATERIAL_INDEX;
        int32 LastMaterialIndex = INVALID_MATERIAL_INDEX;
        while (CurrentIndex < IndexCount)
        {
            Data.MaterialIndex = -1;
            Data.Mesh.Clear();

            UniqueVertices.clear();

            for (; CurrentIndex < IndexCount; CurrentIndex++)
            {
                if (Materials)
                {
                    const uint32 TriangleIndex = (CurrentIndex / 3);
                    if (MaterialIndex != Materials[TriangleIndex])
                    {
                        LastMaterialIndex = MaterialIndex;
                        MaterialIndex     = Materials[TriangleIndex];
                        break;
                    }
                }

                FVertex TempVertex;

                // Position
                FVector3 Position = FVector3(
                    static_cast<float>(Vertices[CurrentIndex].x),
                    static_cast<float>(Vertices[CurrentIndex].y),
                    static_cast<float>(Vertices[CurrentIndex].z));
                TempVertex.Position = Transform.TransformPosition(Position);

                // Apply the scene scale
                if (Flags & EFBXFlags::FBXFlags_ApplyScaleFactor)
                {
                    TempVertex.Position *= UnitScaleFactorRecip;
                }

                // Normal
                FVector3 Normal = FVector3((float)Normals[CurrentIndex].x, (float)Normals[CurrentIndex].y, (float)Normals[CurrentIndex].z);
                TempVertex.Normal = Transform.TransformDirection(Normal);

                // TexCoords
                TempVertex.TexCoord = FVector2((float)TexCoords[CurrentIndex].x, (float)TexCoords[CurrentIndex].y);

                // Tangents
                if (Tangents)
                {
                    FVector3 Tangent = FVector3((float)Tangents[CurrentIndex].x, (float)Tangents[CurrentIndex].y, (float)Tangents[CurrentIndex].z);
                    TempVertex.Tangent = Transform.TransformDirection(Tangent);
                }

                // Only push unique vertices
                uint32 UniqueIndex = 0;
                if (UniqueVertices.count(TempVertex) == 0)
                {
                    UniqueIndex = static_cast<uint32>(Data.Mesh.Vertices.GetSize());
                    UniqueVertices[TempVertex] = UniqueIndex;
                    Data.Mesh.Vertices.Push(TempVertex);
                }
                else
                {
                    UniqueIndex = UniqueVertices[TempVertex];
                }

                Data.Mesh.Indices.Emplace(UniqueIndex);
            }

            if (!Tangents)
            {
                FMeshUtilities::CalculateTangents(Data.Mesh);
            }

            // Convert to left-handed
            if (Flags & EFBXFlags::FBXFlags_EnsureLeftHanded)
            {
                if (Settings->CoordAxis == ofbx::CoordSystem_RightHanded)
                {
                    FMeshUtilities::ReverseHandedness(Data.Mesh);
                }
            }

            Data.Name = CurrentMesh->name;

            if (LastMaterialIndex != INVALID_MATERIAL_INDEX)
            {
                const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial(LastMaterialIndex);
                if (UniqueMaterials.count(CurrentMaterial) != 0)
                {
                    Data.MaterialIndex = UniqueMaterials[CurrentMaterial];
                }
                else
                {
                    Data.MaterialIndex = -1;
                }
            }

            if (Data.Mesh.Hasdata())
            {
                Data.Mesh.RefitContainers();
                OutScene.Models.Emplace(Data);
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
