#include "Core/Math/Math.h"
#include "Core/Containers/Map.h"
#include "Core/Generic/GenericPlatformFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetImporters/OBJImporter.h"

#include <tiny_obj_loader.h>

bool FOBJImporter::ImportFromFile(const StringView& InFilename, EMeshImportFlags /* Flags */, FModelData& OutModelData)
{
    // Load Scene File
    std::string                      Warning;
    std::string                      Error;
    std::vector<tinyobj::shape_t>    Shapes;
    std::vector<tinyobj::material_t> Materials;
    tinyobj::attrib_t                Attributes;

    // Extract just the name of the file
    const String Filename            = String(InFilename);
    const String MTLFiledir          = File::ExtractFilepath(Filename);
    const String FilenameWithoutPath = File::ExtractFilenameWithoutExtension(Filename);
    
    // Load the OBJ file
    if (!tinyobj::LoadObj(&Attributes, &Shapes, &Materials, &Warning, &Error, *Filename, *MTLFiledir, true, false))
    {
        LOG_ERROR("[FOBJImporter]: Failed to load '%s'. Warning: %s Error: %s", *Filename, Warning.c_str(), Error.c_str());
        return false;
    }
    
    if (!Warning.empty())
    {
        LOG_WARNING("[FOBJImporter]: Loaded '%s' with Warning: %s", *Filename, Warning.c_str());
    }

    // Create all Materials in scene
    int32 SceneMaterialIndex = 0;
    for (tinyobj::material_t& Mat : Materials)
    {
        const auto LoadMaterialTexture = [&](const std::string& TexName) -> FTexture2DRef
        {
            if (TexName.empty())
            {
                return nullptr;
            }

            return StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(MTLFiledir + '/' + TexName.c_str()));
        };

        // Create new material with default properties
        FMaterialData MaterialData;
        MaterialData.Textures[EMaterialTexture::Metallic]  = LoadMaterialTexture(Mat.ambient_texname);
        MaterialData.Textures[EMaterialTexture::Diffuse]   = LoadMaterialTexture(Mat.diffuse_texname);
        MaterialData.Textures[EMaterialTexture::Roughness] = LoadMaterialTexture(Mat.specular_highlight_texname);
        MaterialData.Textures[EMaterialTexture::Normal]    = LoadMaterialTexture(Mat.bump_texname);
        MaterialData.Textures[EMaterialTexture::AlphaMask] = LoadMaterialTexture(Mat.alpha_texname);

        MaterialData.Routes[EMaterialScalar::Roughness] = FMaterialSourceRoute(EMaterialTexture::Roughness, ETextureChannel::R);
        MaterialData.Routes[EMaterialScalar::Metallic]  = FMaterialSourceRoute(EMaterialTexture::Metallic, ETextureChannel::R);

        if (MaterialData.Textures[EMaterialTexture::AlphaMask])
        {
            MaterialData.Routes[EMaterialScalar::Opacity] = FMaterialSourceRoute(EMaterialTexture::AlphaMask, ETextureChannel::R);
        }

        MaterialData.Diffuse       = Vector3(Mat.diffuse[0], Mat.diffuse[1], Mat.diffuse[2]);
        MaterialData.Metallic      = Mat.ambient[0];
        MaterialData.AmbientFactor = 1.0f;
        MaterialData.Roughness     = 1.0f;
        MaterialData.MaterialFlags = EMaterialFlags::None;

        if (Mat.name.empty())
        {
            MaterialData.Name = String::CreateFormatted("%s_material_%d", *FilenameWithoutPath, SceneMaterialIndex);
        }
        else
        {
            MaterialData.Name = String(Mat.name.c_str());
        }
        
        OutModelData.Materials.Add(Move(MaterialData));
        SceneMaterialIndex++;
    }

    // Construct Scene
    TMap<FSourceVertex, uint32> UniqueVertices;

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
        FMeshData MeshData;
        MeshData.Indices.Reserve(IndexCount);
        UniqueVertices.Clear();

        uint32 CurrentIndex = 0;
        while (CurrentIndex < IndexCount)
        {
            // Retrieve the matieralIndex
            int32 Face = CurrentIndex / NumIndiciesPerTriangle;
            const int32 MaterialID = Shape.mesh.material_ids[Face];

            // Create a new partition for the mesh
            FSubMesh SubMesh;
            SubMesh.BaseVertex = MeshData.Vertices.Size();
            SubMesh.StartIndex = MeshData.Indices.Size();
            
            if (MaterialID >= 0)
            {
                SubMesh.MaterialIndex = MaterialID;
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

                FSourceVertex Vertex;

                const uint32 PositionIndex = NumPositionsPerTriangle * Index.vertex_index;
                Vertex.Position = Vector3(Attributes.vertices[PositionIndex + 0], Attributes.vertices[PositionIndex + 1], Attributes.vertices[PositionIndex + 2]);

                if (Index.normal_index >= 0)
                {
                    const uint32 NormalIndex = NumNormalsPerTriangle * Index.normal_index;
                    Vertex.Normal = Vector3(Attributes.normals[NormalIndex + 0], Attributes.normals[NormalIndex + 1], Attributes.normals[NormalIndex + 2]);
                    Vertex.Normal.Normalize();
                }

                if (Index.texcoord_index >= 0)
                {
                    const uint32 TexCoordIndex = NumTexCoordsPerTriangle * Index.texcoord_index;
                    Vertex.TexCoord = Vector2(Attributes.texcoords[TexCoordIndex + 0], 1.0f - Attributes.texcoords[TexCoordIndex + 1]);
                }

                uint32 VertexIndex;
                if (uint32* ExistingIndex = UniqueVertices.Find(Vertex))
                {
                    VertexIndex = *ExistingIndex;
                }
                else
                {
                    VertexIndex = static_cast<uint32>(MeshData.Vertices.Size());
                    UniqueVertices[Vertex] = VertexIndex;
                    MeshData.Vertices.Add(Vertex);
                }
                
                MeshData.Indices.Add(VertexIndex);
            }

            // Set the number of vertices/indices for this partition
            SubMesh.VertexCount = MeshData.Vertices.Size() - SubMesh.BaseVertex;
            SubMesh.IndexCount  = MeshData.Indices.Size() - SubMesh.StartIndex;
            MeshData.SubMeshes.Add(SubMesh);
        }

        MeshData.CalculateTangents();

        if (Shape.name.empty())
        {
            MeshData.Name = String::CreateFormatted("%s_%d", *FilenameWithoutPath, ShapeIndex);
        }
        else
        {
            MeshData.Name = Shape.name.c_str();
        }

        OutModelData.Meshes.Add(Move(MeshData));
        ShapeIndex++;
    }

    LOG_INFO("[FOBJImporter]: Loaded Model '%s' which contains %d models and %d materials", *Filename, OutModelData.Meshes.Size(), OutModelData.Materials.Size());
    return true;
}

bool FOBJImporter::MatchExtenstion(const StringView& FileName)
{
    return FileName.EndsWith(".obj", EStringCaseType::NoCase);
}
