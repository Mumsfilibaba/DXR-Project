#include "Core/Math/Matrix4.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformFile.h"
#include "Engine/Assets/VertexFormat.h"
#include "Engine/Assets/AssetImporters/FBXImporter.h"

#include <ofbx.h>

#define INVALID_MATERIAL_INDEX (-1)

static FString ExtractPath(const FString& FullFilePath)
{
    int32 Pos = FullFilePath.FindLastChar('/');
    if (Pos != FString::InvalidIndex)
    {
        return FullFilePath.SubString(0, Pos);
    }
    else
    {
        return FullFilePath;
    }
}

static FMatrix4 FBXConvertMatrix(const ofbx::DMatrix& Matrix)
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

#if 0 // Currently unused
static bool DoesFlipHandness(const FMatrix4& Matrix)
{
    FVector3 X(1.0f, 0.0f, 0.0f);
    X = Matrix.TransformNormal(X);
    
    FVector3 Y(0.0f, 1.0f, 0.0f);
    Y = Matrix.TransformNormal(Y);

    FVector3 Z = Matrix.GetInverse().TransformCoord(X.CrossProduct(Y));
    return Z.Z < 0.0f;
}
#endif

bool FFBXImporter::ImportFromFile(const FStringView& InFilename, EMeshImportFlags InFlags, FImportScene& OutScene)
{
    const FString Filename = FString(InFilename);

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
        const ANSICHAR* ErrorString = ofbx::getError();
        LOG_ERROR("[FFBXImporter]: Failed to load content '%s', error '%s'", *Filename, StringCast<CHAR_T>(ErrorString).Get());
        return false;
    }

    // Array to store indices in when performing triangulation
    TArray<int32> PartitionIndicies;

    // Unique tables
    TMap<uint64, int32>         UniqueMaterials;
    TMap<FString, int32>        UniqueTextures;
    TMap<uint64, int32>         UniqueNodes;
    TMap<uint64, int32>         UniqueGeometries;
    TMap<FSourceVertex, uint32> UniqueVertices;

    FImportScene Scene;
    Scene.Meshes.Reserve(FBXScene->getMeshCount());
    Scene.Nodes.Reserve(FBXScene->getMeshCount());

    // Convert data
    const FString Path = ExtractPath(Filename);

    // Get the global settings
    const ofbx::GlobalSettings* GlobalSettings = FBXScene->getGlobalSettings();

    const bool bApplyScaleFactor    = (InFlags & EMeshImportFlags::ApplyScaleFactor) != EMeshImportFlags::None;
    const bool bReverseHandedness   = (InFlags & EMeshImportFlags::ForceLeftHanded) != EMeshImportFlags::None && GlobalSettings->CoordAxis == ofbx::CoordSystem_RightHanded;
    const bool bInvertAxisX         = (InFlags & EMeshImportFlags::InvertAxisX) != EMeshImportFlags::None;
    const bool bRecalculateTangents = (InFlags & EMeshImportFlags::RecalculateTangents) != EMeshImportFlags::None;

    const auto AddTexture = [&](const ofbx::Material* Material, ofbx::Texture::TextureType Type) -> int32
    {
        const ofbx::Texture* MaterialTexture = Material->getTexture(Type);
        if (!MaterialTexture)
        {
            return -1;
        }

        ANSICHAR StringBuffer[256];
        MaterialTexture->getRelativeFileName().toString(StringBuffer);

        // Make sure that correct slashes are used
        FString TexturePath = Path + '/' + StringCast<CHAR_T>(StringBuffer).ToString();
        TexturePath.ReplaceAll('\\', '/');

        if (const int32* ExistingIndex = UniqueTextures.Find(TexturePath))
        {
            return *ExistingIndex;
        }

        const int32 TextureIndex = Scene.Textures.Size();
        Scene.Textures.Emplace().SourcePath = TexturePath;
        UniqueTextures.Add(TexturePath, TextureIndex);
        return TextureIndex;
    };

    // Parents are added before their children. The scene root isn't a node; its children are the roots.
    TFunction<int32(const ofbx::Object*)> AddNode = [&](const ofbx::Object* Object) -> int32
    {
        if (!Object || Object->getType() == ofbx::Object::Type::ROOT)
        {
            return -1;
        }

        if (const int32* ExistingIndex = UniqueNodes.Find(Object->id))
        {
            return *ExistingIndex;
        }

        const int32 Parent = AddNode(Object->getParent());

        FMatrix4 LocalTransform = FBXConvertMatrix(Object->getLocalTransform());
        if (Parent < 0 && bApplyScaleFactor)
        {
            LocalTransform = LocalTransform * FMatrix4::Scale(GlobalSettings->UnitScaleFactor);
        }

        // The meshes are mirrored in their own space, so the transforms placing them are mirrored to match
        if (bReverseHandedness)
        {
            LocalTransform = ImportScenes::MirrorTransform(LocalTransform, 2);
        }

        if (bInvertAxisX)
        {
            LocalTransform = ImportScenes::MirrorTransform(LocalTransform, 0);
        }

        const int32 NodeIndex = Scene.Nodes.Size();

        FImportNode& Node = Scene.Nodes.Emplace();
        Node.Name           = StringCast<CHAR_T>(Object->name).ToString();
        Node.Parent         = Parent;
        Node.LocalTransform = LocalTransform;

        UniqueNodes.Add(Object->id, NodeIndex);
        return NodeIndex;
    };

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

            FImportMaterial MaterialData;
            MaterialData.Name = StringCast<CHAR_T>(CurrentMaterial->name).ToString();

            MaterialData.Textures[EMaterialTexture::Diffuse]          = AddTexture(CurrentMaterial, ofbx::Texture::TextureType::DIFFUSE);
            MaterialData.Textures[EMaterialTexture::Normal]           = AddTexture(CurrentMaterial, ofbx::Texture::TextureType::NORMAL);
            MaterialData.Textures[EMaterialTexture::Specular]         = AddTexture(CurrentMaterial, ofbx::Texture::TextureType::SPECULAR);
            MaterialData.Textures[EMaterialTexture::Emissive]         = AddTexture(CurrentMaterial, ofbx::Texture::TextureType::EMISSIVE);
            MaterialData.Textures[EMaterialTexture::AmbientOcclusion] = AddTexture(CurrentMaterial, ofbx::Texture::TextureType::AMBIENT);

            if (MaterialData.Textures[EMaterialTexture::Specular] >= 0)
            {
                MaterialData.Routes[EMaterialScalar::Occlusion] = FMaterialSourceRoute(EMaterialTexture::Specular, ETextureChannel::R);
                MaterialData.Routes[EMaterialScalar::Roughness] = FMaterialSourceRoute(EMaterialTexture::Specular, ETextureChannel::G);
                MaterialData.Routes[EMaterialScalar::Metallic]  = FMaterialSourceRoute(EMaterialTexture::Specular, ETextureChannel::B);
            }
            else
            {
                MaterialData.Routes[EMaterialScalar::Occlusion] = FMaterialSourceRoute(EMaterialTexture::AmbientOcclusion, ETextureChannel::R);
            }

            MaterialData.Diffuse       = FVector3(CurrentMaterial->getDiffuseColor().r, CurrentMaterial->getDiffuseColor().g, CurrentMaterial->getDiffuseColor().b);
            MaterialData.AmbientFactor = 1.0f; // CurrentMaterial->getSpecularColor().r;
            MaterialData.Roughness     = 1.0f; // CurrentMaterial->getSpecularColor().g;
            MaterialData.Metallic      = 1.0f; // CurrentMaterial->getSpecularColor().b;

            // TODO: Other material properties
            UniqueMaterials[CurrentMaterial->id] = Scene.Materials.Size();
            Scene.Materials.Add(Move(MaterialData));
        }

        const int32 NodeIndex = AddNode(CurrentMesh);

        const ofbx::GeometryData& GeometryData = CurrentMesh->getGeometryData();
        const int32 PartitionCount = GeometryData.getPartitionCount();

        // A partition draws with the material of the same index on the node, so nodes sharing a geometry can each have their own
        TArray<int32> NodeMaterials;
        for (int32 PartitionIdx = 0; PartitionIdx < PartitionCount; PartitionIdx++)
        {
            int32 MaterialIndex = INVALID_MATERIAL_INDEX;
            if (PartitionIdx < CurrentMesh->getMaterialCount())
            {
                if (const int32* ExistingMaterialIndex = UniqueMaterials.Find(CurrentMesh->getMaterial(PartitionIdx)->id))
                {
                    MaterialIndex = *ExistingMaterialIndex;
                }
            }

            if (MaterialIndex == INVALID_MATERIAL_INDEX)
            {
                LOG_WARNING("[FFBXImporter] Partition in Mesh '%s' has no material", CurrentMesh->name);
            }

            NodeMaterials.Add(MaterialIndex);
        }

        Scene.Nodes[NodeIndex].Materials = ::Move(NodeMaterials);

        // The geometric transform applies to the geometry alone, not to child nodes, so it is part of the mesh. Geometry shared by
        // several nodes is imported once, unless their geometric transforms differ.
        const FMatrix4  GeometricMatrix = FBXConvertMatrix(CurrentMesh->getGeometricMatrix());
        const ofbx::Geometry* Geometry  = CurrentMesh->getGeometry();
        const uint64          SharedKey = (Geometry && GeometricMatrix == FMatrix4::Identity()) ? Geometry->id : CurrentMesh->id;

        if (const int32* ExistingMesh = UniqueGeometries.Find(SharedKey))
        {
            Scene.Nodes[NodeIndex].Mesh = *ExistingMesh;
            continue;
        }

        ofbx::Vec3Attributes Positions = GeometryData.getPositions();
        ofbx::Vec3Attributes Normals   = GeometryData.getNormals();
        ofbx::Vec3Attributes Tangents  = GeometryData.getTangents();
        ofbx::Vec2Attributes TexCoords = GeometryData.getUVs();

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
                    const FVector3 Position(OfbxPosition.x, OfbxPosition.y, OfbxPosition.z);
                    Vertex.Position = GeometricMatrix.Transform(Position);

                    // Normal
                    if (Normals.values)
                    {
                        const ofbx::Vec3 OfbxNormal = Normals.get(VertexIdx);
                        const FVector3 Normal(OfbxNormal.x, OfbxNormal.y, OfbxNormal.z);
                        Vertex.Normal = GeometricMatrix.TransformNormal(Normal);
                    }

                    // Tangents
                    if (Tangents.values)
                    {
                        const ofbx::Vec3 OfbxTangent = Tangents.get(VertexIdx);
                        const FVector3 Tangent(OfbxTangent.x, OfbxTangent.y, OfbxTangent.z);
                        Vertex.Tangent = GeometricMatrix.TransformNormal(Tangent);
                    }

                    // TexCoords
                    if (TexCoords.values)
                    {
                        // We need to correct UVs (I assume since DirectX coordinate system)
                        const ofbx::Vec2 OfbxTexCoord = TexCoords.get(VertexIdx);
                        Vertex.TexCoord = FVector2(OfbxTexCoord.x, 1.0f - OfbxTexCoord.y);
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

            SubMesh.MaterialIndex = PartitionIdx;
        }

        // Convert to left-handed
        if (bReverseHandedness)
        {
            MeshData.ReverseHandedness();
        }

        if (bInvertAxisX)
        {
            MeshData.InvertAxisX();
        }

        // If there are no tangents, then we calculate them
        if (!Tangents.values || bRecalculateTangents)
        {
            MeshData.CalculateTangents();
        }
        else
        {
            MeshData.CalculateTangentSigns();
        }

        // Add the mesh to our scene
        int32 MeshIndex = -1;
        if (!MeshData.Vertices.IsEmpty())
        {
            LOG_INFO("[FFBXImporter] Loaded Mesh '%s'", StringCast<CHAR_T>(CurrentMesh->name).Get());
            MeshData.Name = StringCast<CHAR_T>(CurrentMesh->name).ToString();

            MeshIndex = Scene.Meshes.Size();
            Scene.Meshes.Add(Move(MeshData));
        }
        else
        {
            LOG_WARNING("[FFBXImporter] Tried to load mesh without any data");
        }

        Scene.Nodes[NodeIndex].Mesh = MeshIndex;
        UniqueGeometries.Add(SharedKey, MeshIndex);
    }

    Scene.Meshes.Shrink();
    Scene.Materials.Shrink();

    FBXScene->destroy();

    LOG_INFO("[FFBXImporter]: Loaded Model '%s' which contains %d nodes, %d meshes and %d materials", *Filename, Scene.Nodes.Size(), Scene.Meshes.Size(), Scene.Materials.Size());
    OutScene = ::Move(Scene);
    return true;
}

bool FFBXImporter::MatchExtenstion(const FStringView& FileName)
{
    return FileName.EndsWith(TEXT(".fbx"), EStringCaseType::NoCase);
}
