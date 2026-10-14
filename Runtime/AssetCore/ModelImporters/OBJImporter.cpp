#include "Core/Math/Math.h"
#include "Core/Containers/Map.h"
#include "Core/PlatformInterface/IPlatformFileSystem.h"
#include "Core/Filesystem/File.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "AssetCore/ModelImporters/OBJImporter.h"

#include <tiny_obj_loader.h>
#include <sstream>

static bool ReadText(const FString& Filename, std::string& OutText)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    if (!FileHandle)
    {
        return false;
    }

    TArray<CHAR_T> Text;
    if (!File::ReadTextFile(FileHandle.Get(), Text))
    {
        return false;
    }

    const TStringConversion<ANSICHAR, CHAR_T> Utf8Text = StringCast<ANSICHAR>(Text.Data(), Text.Size() - 1);
    OutText.assign(Utf8Text.Get(), static_cast<size_t>(Utf8Text.Length()));
    return true;
}

class FOBJMaterialReader final : public tinyobj::MaterialReader
{
public:
    explicit FOBJMaterialReader(const FString& InDirectory)
        : Directory(InDirectory)
    {
    }

    virtual bool operator()(const std::string& MaterialFile, std::vector<tinyobj::material_t>* Materials, std::map<std::string, int>* MaterialMap, std::string* Warning, std::string* Error) override final
    {
        const FString Filename = File::CombinePath(Directory, StringCast<CHAR_T>(MaterialFile.c_str(), static_cast<int32>(MaterialFile.size())).ToString());

        std::string Text;
        if (!ReadText(Filename, Text))
        {
            if (Warning)
            {
                *Warning += "Material file [ " + MaterialFile + " ] not found\n";
            }

            return false;
        }

        std::istringstream Stream(Text);
        tinyobj::LoadMtl(MaterialMap, Materials, &Stream, Warning, Error);
        return true;
    }

private:
    FString Directory;
};

bool FOBJImporter::ImportFromFile(const FStringView& InFilename, EMeshImportFlags /* Flags */, FImportScene& OutScene)
{
    // Load Scene File
    std::string                      Warning;
    std::string                      Error;
    std::vector<tinyobj::shape_t>    Shapes;
    std::vector<tinyobj::material_t> Materials;
    tinyobj::attrib_t                Attributes;

    // Extract just the name of the file
    const FString Filename            = FString(InFilename);
    const FString MTLFiledir          = File::GetDirectoryOf(Filename);
    const FString FilenameWithoutPath = File::ExtractFilenameWithoutExtension(Filename);

    // Load the OBJ file
    std::string Text;
    if (!ReadText(Filename, Text))
    {
        LOG_ERROR("[FOBJImporter]: Failed to open '%s'", *Filename);
        return false;
    }

    std::istringstream TextStream(Text);
    FOBJMaterialReader MaterialReader(MTLFiledir);
    if (!tinyobj::LoadObj(&Attributes, &Shapes, &Materials, &Warning, &Error, &TextStream, &MaterialReader, true, false))
    {
        LOG_ERROR("[FOBJImporter]: Failed to load '%s'. Warning: %s Error: %s", *Filename, StringCast<CHAR_T>(Warning.c_str()).Get(), StringCast<CHAR_T>(Error.c_str()).Get());
        return false;
    }

    if (!Warning.empty())
    {
        LOG_WARNING("[FOBJImporter]: Loaded '%s' with Warning: %s", *Filename, StringCast<CHAR_T>(Warning.c_str()).Get());
    }

    FImportScene         Scene;
    TMap<FString, int32> TextureIndices;

    const auto AddTexture = [&](const std::string& TexName) -> int32
    {
        if (TexName.empty())
        {
            return -1;
        }

        FString TexturePath = MTLFiledir + '/' + StringCast<CHAR_T>(TexName.c_str()).ToString();
        TexturePath.ReplaceAll('\\', '/');

        if (const int32* ExistingIndex = TextureIndices.Find(TexturePath))
        {
            return *ExistingIndex;
        }

        const int32 TextureIndex = Scene.Textures.Size();
        Scene.Textures.Emplace().SourcePath = TexturePath;
        TextureIndices.Add(TexturePath, TextureIndex);
        return TextureIndex;
    };

    // Create all Materials in scene
    int32 SceneMaterialIndex = 0;
    for (tinyobj::material_t& Mat : Materials)
    {
        FImportMaterial& MaterialData = Scene.Materials.Emplace();
        MaterialData.Textures[EMaterialTexture::Metallic]  = AddTexture(Mat.ambient_texname);
        MaterialData.Textures[EMaterialTexture::Diffuse]   = AddTexture(Mat.diffuse_texname);
        MaterialData.Textures[EMaterialTexture::Roughness] = AddTexture(Mat.specular_highlight_texname);
        MaterialData.Textures[EMaterialTexture::Normal]    = AddTexture(Mat.bump_texname);
        MaterialData.Textures[EMaterialTexture::AlphaMask] = AddTexture(Mat.alpha_texname);

        MaterialData.Routes[EMaterialScalar::Roughness] = FMaterialSourceRoute(EMaterialTexture::Roughness, ETextureChannel::R);
        MaterialData.Routes[EMaterialScalar::Metallic]  = FMaterialSourceRoute(EMaterialTexture::Metallic, ETextureChannel::R);

        if (MaterialData.Textures[EMaterialTexture::AlphaMask] >= 0)
        {
            MaterialData.Routes[EMaterialScalar::Opacity] = FMaterialSourceRoute(EMaterialTexture::AlphaMask, ETextureChannel::R);
        }

        MaterialData.Diffuse       = FVector3(Mat.diffuse[0], Mat.diffuse[1], Mat.diffuse[2]);
        MaterialData.Metallic      = Mat.ambient[0];
        MaterialData.AmbientFactor = 1.0f;
        MaterialData.Roughness     = 1.0f;
        MaterialData.MaterialFlags = EMaterialFlags::None;

        if (Mat.name.empty())
        {
            MaterialData.Name = FString::Printf(TEXT("%s_material_%d"), *FilenameWithoutPath, SceneMaterialIndex);
        }
        else
        {
            MaterialData.Name = StringCast<CHAR_T>(Mat.name.c_str()).ToString();
        }

        SceneMaterialIndex++;
    }

    // Every object and group becomes a node with an identity transform, placing its own mesh
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

        // Start a new mesh, whose submeshes pick their materials through the node's slots
        FMeshData   MeshData;
        FImportNode Node;
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
                SubMesh.MaterialIndex = Node.Materials.Find(MaterialID);
                if (SubMesh.MaterialIndex < 0)
                {
                    SubMesh.MaterialIndex = Node.Materials.Size();
                    Node.Materials.Add(MaterialID);
                }
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
                    Vertex.TexCoord = FVector2(Attributes.texcoords[TexCoordIndex + 0], 1.0f - Attributes.texcoords[TexCoordIndex + 1]);
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
            MeshData.Name = FString::Printf(TEXT("%s_%d"), *FilenameWithoutPath, ShapeIndex);
        }
        else
        {
            MeshData.Name = StringCast<CHAR_T>(Shape.name.c_str()).ToString();
        }

        Node.Name = MeshData.Name;
        Node.Mesh = Scene.Meshes.Size();
        Scene.Nodes.Add(::Move(Node));
        Scene.Meshes.Add(::Move(MeshData));
        ShapeIndex++;
    }

    LOG_INFO("[FOBJImporter]: Loaded Model '%s' which contains %d models and %d materials", *Filename, Scene.Meshes.Size(), Scene.Materials.Size());
    OutScene = ::Move(Scene);
    return true;
}

bool FOBJImporter::MatchExtenstion(const FStringView& FileName)
{
    return FileName.EndsWith(TEXT(".obj"), EStringCaseType::NoCase);
}
